#include <map>

#include <obfuscator/context.hpp>

#include <zasm/formatter/formatter.hpp>
#include <fmt/format.h>

namespace obfuscator
{

using namespace zasm;

static std::map<uint32_t, const char*> flagToStringMap = {
    {ZYDIS_CPUFLAG_CF, "CF"},
    {ZYDIS_CPUFLAG_PF, "PF"},
    {ZYDIS_CPUFLAG_AF, "AF"},
    {ZYDIS_CPUFLAG_ZF, "ZF"},
    {ZYDIS_CPUFLAG_SF, "SF"},
    {ZYDIS_CPUFLAG_TF, "TF"},
    {ZYDIS_CPUFLAG_IF, "IF"},
    {ZYDIS_CPUFLAG_DF, "DF"},
    {ZYDIS_CPUFLAG_OF, "OF"},
    {ZYDIS_CPUFLAG_NT, "NT"},
    {ZYDIS_CPUFLAG_RF, "RF"},
    {ZYDIS_CPUFLAG_VM, "VM"},
    {ZYDIS_CPUFLAG_AC, "AC"},
    {ZYDIS_CPUFLAG_VIF, "VIF"},
    {ZYDIS_CPUFLAG_VIP, "VIP"},
    {ZYDIS_CPUFLAG_ID, "ID"},
};

const char* flagToString(uint32_t flag)
{
    auto itr = flagToStringMap.find(flag);
    return itr == flagToStringMap.end() ? nullptr : itr->second;
}

std::vector<uint32_t> maskToFlags(uint32_t mask)
{
    std::vector<uint32_t> result;
    for (auto& [flag, name] : flagToStringMap)
    {
        if (mask & flag)
        {
            result.push_back(flag);
        }
    }
    return result;
}

std::string formatFlagsMask(uint32_t mask)
{
    std::string result;
    for (auto& [flag, name] : flagToStringMap)
    {
        if (mask & flag)
        {
            if (!result.empty())
            {
                result += " ";
            }
            result += name;
        }
    }
    return "(" + result + ")";
}

std::string formatRegsMask(uint64_t mask)
{
    std::string result;
#define REG(x)                                     \
    if (mask & (1ULL << (x - ZYDIS_REGISTER_RAX))) \
    result += (&(#x)[15]), result += " "
    REG(ZYDIS_REGISTER_RAX);
    REG(ZYDIS_REGISTER_RBX);
    REG(ZYDIS_REGISTER_RCX);
    REG(ZYDIS_REGISTER_RDX);
    REG(ZYDIS_REGISTER_RSP);
    REG(ZYDIS_REGISTER_RBP);
    REG(ZYDIS_REGISTER_RSI);
    REG(ZYDIS_REGISTER_RDI);
    REG(ZYDIS_REGISTER_R8);
    REG(ZYDIS_REGISTER_R9);
    REG(ZYDIS_REGISTER_R10);
    REG(ZYDIS_REGISTER_R11);
    REG(ZYDIS_REGISTER_R12);
    REG(ZYDIS_REGISTER_R13);
    REG(ZYDIS_REGISTER_R14);
    REG(ZYDIS_REGISTER_R15);
#undef REG
    if (!result.empty())
        result.pop_back();
    return "(" + result + ")";
}

std::vector<x86::Gp> maskToRegs(uint64_t mask)
{
    std::vector<x86::Gp> result;
#define REG(x)                                     \
    if (mask & (1ULL << (x - ZYDIS_REGISTER_RAX))) \
        result.emplace_back(static_cast<Reg::Id>(x));
    REG(ZYDIS_REGISTER_RAX);
    REG(ZYDIS_REGISTER_RBX);
    REG(ZYDIS_REGISTER_RCX);
    REG(ZYDIS_REGISTER_RDX);
    REG(ZYDIS_REGISTER_RSP);
    REG(ZYDIS_REGISTER_RBP);
    REG(ZYDIS_REGISTER_RSI);
    REG(ZYDIS_REGISTER_RDI);
    REG(ZYDIS_REGISTER_R8);
    REG(ZYDIS_REGISTER_R9);
    REG(ZYDIS_REGISTER_R10);
    REG(ZYDIS_REGISTER_R11);
    REG(ZYDIS_REGISTER_R12);
    REG(ZYDIS_REGISTER_R13);
    REG(ZYDIS_REGISTER_R14);
    REG(ZYDIS_REGISTER_R15);
#undef REG
    return result;
}

uint32_t regMask(const Reg& reg)
{
    if (!reg.isValid() || reg == x86::rip || reg == x86::rflags)
    {
        return 0;
    }

    if (!reg.isGp())
    {
        auto regText = formatter::toString(reg);
        fmt::println("\tunsupported register type {}", regText);
        return 0;
    }

    return 1u << reg.getIndex();
}

InstructionData*
Context::addInstructionData(Node* node, uint64_t address, MachineMode mode, const InstructionDetail& detail)
{
    auto data = node->getUserData<InstructionData>();
    if (data == nullptr)
    {
        instructionDataPool.emplace_back();
        data          = &instructionDataPool.back();
        data->address = address;
        data->detail  = detail;

        // Populate the registers read and written by the instruction
        uint32_t regsRead    = 0;
        uint32_t regsWritten = 0;
        for (size_t i = 0; i < detail.getOperandCount(); i++)
        {
            const auto& operand = detail.getOperand(i);
            if (auto reg = operand.getIf<Reg>())
            {
                auto access = detail.getOperandAccess(i);
                if ((uint8_t)(access & Operand::Access::MaskRead))
                {
                    data->regsRead |= regMask(reg->getRoot(mode));
                }
                if ((uint8_t)(access & Operand::Access::MaskWrite))
                {
                    // mov al, 66 does not kill rax
                    if (reg->isGp32() || reg->isGp64())
                    {
                        data->regsWritten |= regMask(reg->getRoot(mode));
                    }
                }
            }
            else if (auto mem = operand.getIf<Mem>())
            {
                data->regsRead |= regMask(mem->getBase().getRoot(mode));
                data->regsRead |= regMask(mem->getIndex().getRoot(mode));
            }
        }

        // Populate the flags modified and tested by the instruction
        const auto& flags   = detail.getCPUFlags();
        data->flagsModified = flags.set0 | flags.set1 | flags.modified | flags.undefined;
        data->flagsTested   = flags.tested;

        // Special handling for call and ret instructions to properly support the calling conventions
        // https://learn.microsoft.com/en-us/cpp/build/x64-software-conventions?view=msvc-170#x64-register-usage
        switch (detail.getCategory())
        {
        case x86::Category::Call:
        {
            // The call instruction clobbers all volatile registers
            data->regsWritten = regMask(x86::rax) | regMask(x86::rcx) | regMask(x86::rdx) | regMask(x86::r8)
                              | regMask(x86::r9) | regMask(x86::r10) | regMask(x86::r11);
            // The call instruction reads the first 4 arguments from rcx, rdx, r8, r9 (and rsp because mishap said so)
            data->regsRead = regMask(x86::rcx) | regMask(x86::rdx) | regMask(x86::r8) | regMask(x86::r9)
                           | regMask(x86::rsp);
        }
        break;

        case x86::Category::Ret:
        {
            // The ret instruction 'reads' all non-volatile registers and the return value
            data->regsRead = regMask(x86::rsp) | regMask(x86::rbx) | regMask(x86::rbp) | regMask(x86::rsi)
                           | regMask(x86::rdi) | regMask(x86::r12) | regMask(x86::r13) | regMask(x86::r14)
                           | regMask(x86::r15) | regMask(x86::rax);
        }
        break;
        }

        node->setUserData(data);
    }
    return data;
}

} // namespace obfuscator
