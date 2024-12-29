#include <gtest/gtest.h>

#include <obfuscator/msvc-secure.hpp>
#include <obfuscator/analyze.hpp>
#include <zasm/formatter/formatter.hpp>

using namespace zasm::x86;
using namespace zasm;
using namespace obfuscator;

static void assignInstructionData(ObfuscationProgram& program, uint64_t fakeAddress, bool verbose = false)
{
    auto mode = program.getMode();

    // NOTE: The analysis expects the last node to be a label (not nullptr)
    Assembler assembler(program);
    assembler.setCursor(program.getTail());
    assembler.bind(assembler.createLabel("__end"));

    for (auto node = program.getNodeForLabel(program.getEntryPoint()); node != nullptr; node = node->getNext())
    {
        if (auto instr = node->getIf<Instruction>())
        {
            if (verbose)
            {
                fmt::println("[{}] {}", fakeAddress, formatter::toString(program, node));
            }

            auto address = fakeAddress++;
            auto detail  = *instr->getDetail(mode);
            program.addInstructionData(node, address, mode, detail);

            // NOTE: The analysis expects the fallthrough block to have a label
            if (detail.getCategory() == x86::Category::CondBr && node->getNext()->getIf<Label>() == nullptr)
            {
                assembler.setCursor(node);
                assembler.bind(assembler.createLabel());
            }
        }
        else if (auto label = node->getIf<Label>())
        {
            if (verbose)
            {
                fmt::println("\n[{}] {}", fakeAddress, formatter::toString(program, node));
            }
            // NOTE: Labels take the address of the next instruction
            program.addInstructionData(node, fakeAddress, mode, {});
        }
        else
        {
            fmt::println("Unexpected node: [{}] {}", fakeAddress, formatter::toString(program, node));
            __debugbreak();
        }
    }
}

TEST(Analyze, CFG)
{
    ObfuscationProgram   program(MachineMode::AMD64);
    zasm::x86::Assembler assembler(program);
    auto                 entry  = assembler.createLabel("entry");
    auto                 hacker = assembler.createLabel("hacker");
    auto                 end    = assembler.createLabel("end");

    assembler.bind(entry);
    assembler.mov(rax, rcx);
    assembler.cmp(rax, Imm64(0x1337));
    assembler.jz(hacker);

    assembler.mov(rax, Imm64(0x12));
    assembler.jmp(end);

    assembler.bind(hacker);
    assembler.mov(rax, Imm64(0x34));

    assembler.bind(end);
    assembler.bswap(rax);
    assembler.ret();

    program.setEntryPoint(entry);
    assignInstructionData(program, 100);

    auto nodeAddress = [&program](Node* node)
    {
        auto instructionData = node->getUserData<InstructionData>();
        EXPECT_NE(instructionData, nullptr);
        return instructionData->address;
    };

    auto labelAddress = [&program, &nodeAddress](Label label) -> uint64_t
    {
        return nodeAddress(program.getNodeForLabel(label));
    };

    ASSERT_EQ(labelAddress(entry), 100);
    ASSERT_EQ(labelAddress(hacker), 105);
    ASSERT_EQ(labelAddress(end), 106);

    auto cfg = obfuscator::CFG::analyze(program, program.getEntryPoint());
    ASSERT_EQ(cfg->exits, std::set<uint64_t>{106});

    const auto& blocks = cfg->blocks;
    ASSERT_EQ(blocks.size(), 4);
    ASSERT_TRUE(blocks.contains(100));
    ASSERT_TRUE(blocks.contains(103));
    ASSERT_TRUE(blocks.contains(105));
    ASSERT_TRUE(blocks.contains(106));

    const auto& bb100 = blocks.at(100);
    const auto& bb103 = blocks.at(103);
    const auto& bb105 = blocks.at(105);
    const auto& bb106 = blocks.at(106);

    ASSERT_EQ(bb100.address, 100);
    ASSERT_EQ(nodeAddress(bb100.begin), 100);
    ASSERT_EQ(nodeAddress(bb100.end), 103);
    ASSERT_EQ(bb100.label, entry);
    ASSERT_EQ(bb100.successors.size(), 2);
    ASSERT_EQ(bb100.successors[0], bb105.label);
    ASSERT_EQ(bb100.successors[1], bb103.label);

    ASSERT_EQ(bb103.address, 103);
    ASSERT_EQ(nodeAddress(bb103.begin), 103);
    ASSERT_EQ(nodeAddress(bb103.end), 105);
    ASSERT_EQ(bb103.successors.size(), 1);
    ASSERT_EQ(bb103.successors[0], bb106.label);

    ASSERT_EQ(bb105.address, 105);
    ASSERT_EQ(nodeAddress(bb105.begin), 105);
    ASSERT_EQ(nodeAddress(bb105.end), 106);
    ASSERT_EQ(bb105.label, hacker);
    ASSERT_EQ(bb105.successors.size(), 1);
    ASSERT_EQ(bb105.successors[0], bb106.label);

    ASSERT_EQ(bb106.address, 106);
    ASSERT_EQ(nodeAddress(bb106.begin), 106);
    ASSERT_EQ(nodeAddress(bb106.end), 108);
    ASSERT_EQ(bb106.label, end);
    ASSERT_EQ(bb106.successors.size(), 0);
}
