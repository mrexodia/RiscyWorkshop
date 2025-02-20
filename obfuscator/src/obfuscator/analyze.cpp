#include <obfuscator/analyze.hpp>
#include <obfuscator/msvc-secure.hpp>

#include <zasm/formatter/formatter.hpp>
#include <fmt/format.h>

#include <set>
#include <map>
#include <queue>

namespace obfuscator
{

using namespace zasm;

Expected<CFG, std::string> CFG::analyze(const zasm::Program& program, Label entry, bool verbose)
{
    CFG cfg(program);

    if (verbose)
    {
        fmt::println("=== ANALYZE ===");
    }
    std::vector<Label> queue;
    queue.push_back(entry);
    std::set<Label::Id> visited;

    while (!queue.empty())
    {
        auto blockStartLabel = queue.back();
        queue.pop_back();

        if (visited.count(blockStartLabel.getId()))
        {
            continue;
        }
        visited.insert(blockStartLabel.getId());

        const auto& labelData    = *program.getLabelData(blockStartLabel);
        auto        blockAddress = labelData.node->getUserData<InstructionData>()->address;

        BasicBlock bb;
        bb.address = blockAddress;
        bb.label   = blockStartLabel;
        bb.begin   = labelData.node->getNext();
        if (bb.begin == nullptr)
        {
            fmt::println("empty block!");
            __debugbreak();
        }
        if (verbose)
        {
            auto name = labelData.name != nullptr ? labelData.name : "";
            fmt::println("<==> Disassembling block: {} ({:#x})", name, blockAddress);
        }

        auto node     = labelData.node->getNext();
        bool finished = false;
        while (!finished)
        {
            auto instr = node->getIf<Instruction>();
            if (instr == nullptr)
            {
                // End label
                if (node->getNext() == nullptr)
                {
                    break;
                }

                auto label = node->getIf<Label>();
                if (label == nullptr)
                {
                    fmt::println("not label!");
                    __debugbreak();
                }
                
                queue.push_back(*label);
                bb.successors.push_back(*label);
                if (verbose)
                {
                    fmt::println("not instr!");
                }
                break;
            }

            auto data = node->getUserData<InstructionData>();
            if (verbose)
            {
                auto str = formatter::toString(program, instr, formatter::Options::HexImmediates);
                fmt::println("{:#x}|{}", data->address, str);
            }

            switch (data->detail.getCategory())
            {
            case x86::Category::UncondBR:
            {
                auto dest = instr->getOperand<Label>(0);
                if (verbose)
                {
                    fmt::println("UncondBR: {}", dest.getId());
                }
                queue.push_back(dest);
                bb.successors.push_back(dest);
                finished = true;
            }
            break;

            case x86::Category::CondBr:
            {
                auto brtrue  = instr->getOperand<Label>(0);
                auto brfalse = node->getNext()->get<Label>();
                if (verbose)
                {
                    fmt::println("CondBr: {}, {}", brtrue.getId(), brfalse.getId());
                }
                queue.push_back(brfalse);
                queue.push_back(brtrue);
                bb.successors.push_back(brtrue);
                bb.successors.push_back(brfalse);
                finished = true;
            }
            break;

            case x86::Category::Call:
            {
                auto dest = instr->getOperand(0);
                if (dest.getIf<Label>() != nullptr)
                {
                    return makeUnexpected(fmt::format("unsupported call imm {:#x}", data->address));
                }
            }
            break;

            case x86::Category::Ret:
            {
                finished = true;
                cfg.exits.insert(bb.address);
            }
            break;

            default:
            {
            }
            break;
            }

            node = node->getNext();
        }

        // NOTE: We do not allow a nullptr end node, because we need to walk the blocks backwards
        if (node == nullptr)
        {
            fmt::println("empty block!");
            __debugbreak();
        }

        bb.end = node;

        cfg.blocks.emplace(bb.address, bb);
    }

    return cfg;
}

std::map<uint64_t, std::set<uint64_t>> CFG::getPredecessors() const
{
    std::map<uint64_t, std::set<uint64_t>> predecessors;
    for (const auto& [address, _] : blocks)
    {
        predecessors[address] = {};
    }

    for (const auto& [address, block] : blocks)
    {
        for (const auto& successor : block.successors)
        {
            auto data = program.getLabelData(successor).value().node->getUserData<InstructionData>();
            auto successorAddress = data->address;
            predecessors[successorAddress].insert(address);
        }
    }

    return predecessors;
}

std::map<uint64_t, BlockLiveness> CFG::getBlockLiveness(bool verbose) const
{
    // Perform liveness analysis on the control flow graph
    // https://en.wikipedia.org/wiki/Live-variable_analysis

    // TODO: confirm this statement
    // water: "Dominator tree would not work for this, because it
    // does not take into account the next iteration of the loop"

    // Compute the GEN and KILL sets for each block
    std::map<uint64_t, BlockLiveness> blockLiveness;
    for (auto& [address, block] : blocks)
    {
        if (verbose)
        {
            auto str = formatter::toString(program, block.begin, block.end, formatter::Options::HexImmediates);
            fmt::println("Analyzing block {:#x}\n==========\n{}\n==========", address, str);
        }

        auto& liveness = blockLiveness[address];
        for (auto node = block.begin; node != block.end; node = node->getNext())
        {
            auto  data   = node->getUserData<InstructionData>();
            auto& detail = data->detail;

            if (verbose)
            {
                auto instrText = formatter::toString(program, node, formatter::Options::HexImmediates);
                fmt::println("{:#x}|{}\n", data->address, instrText);
                fmt::println("\tregs read: {}", formatRegsMask(data->regsRead));
                fmt::println("\tregs written: {}", formatRegsMask(data->regsWritten));
                fmt::println("\tflags tested: {}", formatFlagsMask(data->flagsTested));
                fmt::println("\tflags modified: {}", formatFlagsMask(data->flagsModified));
            }

            liveness.regsGen |= data->regsRead & ~liveness.regsKill;
            liveness.regsKill |= data->regsWritten;
            liveness.flagsGen  = liveness.flagsGen | (data->flagsTested & ~liveness.flagsKill);
            liveness.flagsKill = liveness.flagsKill | data->flagsModified;
        }

        if (verbose)
        {
            fmt::println("regs_gen: {}", formatRegsMask(liveness.regsGen));
            fmt::println("regs_kill: {}", formatRegsMask(liveness.regsKill));
            fmt::println("flags_gen: {}", formatFlagsMask(liveness.flagsGen));
            fmt::println("flags_kill: {}", formatFlagsMask(liveness.flagsKill));
        }
    }

    auto predecessors = getPredecessors();

    // Solve the dataflow equations
    std::queue<uint64_t> worklist;
    for (auto exit : exits)
    {
        worklist.push(exit);
    }
    while (!worklist.empty())
    {
        auto address = worklist.front();
        worklist.pop();

        auto&         liveness       = blockLiveness.at(address);
        auto          newRegsLiveIn  = liveness.regsGen | (liveness.regsLiveOut & ~liveness.regsKill);
        InstrCPUFlags newFlagsLiveIn = liveness.flagsGen | (liveness.flagsLiveOut & ~liveness.flagsKill);
        if (newRegsLiveIn != liveness.regsLiveIn || newFlagsLiveIn != liveness.flagsLiveIn)
        {
            // Update the LIVEin sets
            liveness.regsLiveIn  = newRegsLiveIn;
            liveness.flagsLiveIn = newFlagsLiveIn;

            // Update the LIVEout sets in the predecessors and add them to the worklist
            for (const auto& predecessor : predecessors.at(address))
            {
                auto& predecessorBlock = blockLiveness.at(predecessor);
                predecessorBlock.regsLiveOut |= newRegsLiveIn;
                predecessorBlock.flagsLiveOut = predecessorBlock.flagsLiveOut | newFlagsLiveIn;
                worklist.push(predecessor);
            }
        }
    }

    return blockLiveness;
}

std::vector<InstructionLiveness>
CFG::getInstructionLiveness(const std::map<uint64_t, BlockLiveness>& livenessBlocks, bool verbose) const
{
    // Compute liveness backwards for each block individually
    std::vector<InstructionLiveness> result;
    for (auto& [address, block] : blocks)
    {
        if (verbose)
        {
            auto str = formatter::toString(program, block.begin, block.end, formatter::Options::HexImmediates);
            fmt::println("Analyzing block {:#x}\n==========\n{}\n==========", address, str);
        }

        // We start with the live-out set of the block
        auto&         liveness  = livenessBlocks.at(address);
        InstrCPUFlags flagsLive = liveness.flagsLiveOut;
        uint32_t      regsLive  = liveness.regsLiveOut;
        for (auto node = block.end->getPrev(); node != block.begin->getPrev(); node = node->getPrev())
        {
            auto  data   = node->getUserData<InstructionData>();
            auto& detail = data->detail;

            auto flagsModified = data->flagsModified;
            auto flagsTested   = data->flagsTested;
            auto regsRead      = data->regsRead;
            auto regsWritten   = data->regsWritten;

            if (verbose)
            {
                auto instrText = formatter::toString(program, node, formatter::Options::HexImmediates);
                fmt::println("{:#x}|{}", data->address, instrText);
                fmt::println("\tflags modified: {}", formatFlagsMask(flagsModified));
                fmt::println("\tflags tested: {}", formatFlagsMask(flagsTested));
                fmt::println("\tregs read: {}", formatRegsMask(regsRead));
                fmt::println("\tregs written: {}", formatRegsMask(regsWritten));

                if (flagsModified & flagsLive)
                {
                    fmt::println("\tlive flags are modified: {}", formatFlagsMask(flagsModified & flagsLive));
                }

                if (flagsTested & flagsLive)
                {
                    fmt::println("\tlive flags are tested: {}", formatFlagsMask(flagsTested & flagsLive));
                }
            }

            // If the flag is tested, it becomes live
            if (flagsTested)
            {
                flagsLive = flagsLive | flagsTested;
                if (verbose)
                    fmt::println("\tnew live flags: {}", formatFlagsMask(flagsLive));
            }

            if (regsRead)
            {
                regsLive = regsLive | regsRead;
                if (verbose)
                    fmt::println("\tnew live regs: {}", formatRegsMask(regsLive));
            }

            // Store the liveness state for the instruction
            result.push_back({node, regsLive, flagsLive});

            if (flagsModified)
            {
                // If the flag is overwritten, it becomes dead
                flagsLive = flagsLive & ~(flagsModified & ~flagsTested);
            }

            if (regsWritten)
            {
                // If the register is overwritten, it becomes dead
                // This fixes a special case where a register is both read and written by
                // the same instruction, which would otherwise cause incorrectly to be marked as dead
                regsLive = regsLive & ~(regsWritten & ~regsRead);
            }

            if (verbose)
            {
                fmt::println("\tfinal live flags: {}", formatFlagsMask(data->flagsLive));
                fmt::println("\tfinal live regs: {}", formatRegsMask(data->regsLive));
            }
        }
    }

    return result;
}

} // namespace obfuscator
