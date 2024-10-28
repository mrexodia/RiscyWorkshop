#pragma once

#include <map>
#include <set>
#include <vector>

#include <obfuscator/context.hpp>

namespace obfuscator
{
struct BasicBlock
{
    uint64_t                 address = 0;
    zasm::Label              label;
    zasm::Node*              begin = nullptr;
    zasm::Node*              end   = nullptr;
    std::vector<zasm::Label> successors;
};

struct BlockLiveness
{
    uint32_t regsGen     = 0;
    uint32_t regsKill    = 0;
    uint32_t regsLiveIn  = 0;
    uint32_t regsLiveOut = 0;

    zasm::InstrCPUFlags flagsGen     = 0;
    zasm::InstrCPUFlags flagsKill    = 0;
    zasm::InstrCPUFlags flagsLiveIn  = 0;
    zasm::InstrCPUFlags flagsLiveOut = 0;
};

struct InstructionLiveness
{
    zasm::Node*         node      = nullptr;
    uint32_t            regsLive  = 0;
    zasm::InstrCPUFlags flagsLive = 0;
};

class CFG
{
    explicit CFG(const zasm::Program& program) : program(program)
    {
    }

  public:
    const zasm::Program&           program;
    std::map<uint64_t, BasicBlock> blocks;
    std::set<uint64_t>             exits;

    static zasm::Expected<CFG, std::string>
    analyze(const zasm::Program& program, zasm::Label entry, bool verbose = false);

    std::map<uint64_t, std::set<uint64_t>> getPredecessors() const;
    std::map<uint64_t, BlockLiveness>      getLivenessBlocks(bool verbose = false) const;
    std::vector<InstructionLiveness>
    getInstructionLiveness(const std::map<uint64_t, BlockLiveness>& blockLiveness, bool verbose = false) const;
};

} // namespace obfuscator
