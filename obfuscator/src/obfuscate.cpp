#include <fstream>
#include <iostream>
#include <cstdlib>
#include <vector>

#include <obfuscator/msvc-secure.hpp>
#include <obfuscator/utility.hpp>

#include <zasm/zasm.hpp>
#include <zasm/formatter/formatter.hpp>

#include <fmt/format.h>
#include <args.hpp>
#include <nlohmann/json.hpp>

using namespace zasm;
using namespace obfuscator;

#include <obfuscator/program.hpp>
#include <obfuscator/disassemble.hpp>
#include <obfuscator/analyze.hpp>
#include <obfuscator/obfuscate.hpp>

struct Arguments : ArgumentParser
{
    std::string input;
    std::string output;
    std::string cleanOutput;
    bool        verbose = false;

    Arguments(int argc, char** argv) : ArgumentParser("Obfuscates the riscvm_run function")
    {
        addPositional("input", input, "Input PE file to obfuscate", true);
        addString("-output", output, "Obfuscated function binary blob");
        addString("-clean-output", cleanOutput, "Unobfuscated function binary blob");
        addBool("-verbose", verbose, "Verbose output");
        parseOrExit(argc, argv);
    }
};

static nlohmann::json livenessJson(uint64_t regsLive, InstrCPUFlags flagsLive)
{
    nlohmann::json json;

    std::vector<std::string> regs;
    for (const auto& reg : maskToRegs(regsLive))
    {
        auto name = formatter::toString(reg);
        for (auto& ch : name)
        {
            if (ch >= 'a' && ch <= 'z')
            {
                ch = toupper(ch);
            }
        }
        regs.push_back(std::move(name));
    }
    json["regs"] = regs;

    std::vector<std::string> flags;
    for (const auto& flag : maskToFlags(flagsLive))
    {
        auto name = flagToString(flag);
        if (name != nullptr)
        {
            flags.push_back(name);
        }
    }
    json["flags"] = flags;

    return json;
}

static void dumpLiveness(CFG& cfg, const std::map<uint64_t, BlockLiveness>& livenessBlocks)
{
    nlohmann::json json;
    // Print the results
    std::string script;
    for (const auto& [address, block] : cfg.blocks)
    {
        auto& liveness = livenessBlocks.at(address);

        nlohmann::json blockJson;
        blockJson["Liveness in"]  = livenessJson(liveness.regsLiveIn, liveness.flagsLiveIn);
        blockJson["Liveness out"] = livenessJson(liveness.regsLiveOut, liveness.flagsLiveOut);

        nlohmann::json instrJson;
        fmt::println("Results for block {:#x}\n==========", address);
        for (auto node = block.begin; node != block.end; node = node->getNext())
        {
            auto data = node->getUserData<InstructionData>();
            auto str  = formatter::toString(cfg.program, node, formatter::Options::HexImmediates);

            instrJson[fmt::format("{:#x}", data->address)] = livenessJson(data->regsLive, data->flagsLive);

            script += "commentset ";
            char address[32];
            sprintf_s(address, "0x%llX", data->address);
            script += address;
            script += ", \"";
            if (data->regsLive || data->flagsLive)
            {
                script += formatRegsMask(data->regsLive);
                if (data->flagsLive)
                {
                    script += "|";
                    script += formatFlagsMask(data->flagsLive);
                }
            }
            else
            {
                script += "no live (HA)";
            }
            script += "\"\n";

            fmt::println(
                "{:#x}|{}|{}|{}", data->address, str, formatRegsMask(data->regsLive), formatFlagsMask(data->flagsLive)
            );

            if (data->regsRead & ~data->regsLive)
            {
                fmt::println("\tdead regs read: %s\n", formatRegsMask(data->regsRead & ~data->regsLive).c_str());
                __debugbreak();
            }
        }
        fmt::println("==========");

        auto blockLiveness = livenessBlocks.at(address);
        fmt::println("\tregs_live_in: {}", formatRegsMask(blockLiveness.regsLiveIn));
        fmt::println("\tregs_live_out: {}", formatRegsMask(blockLiveness.regsLiveOut));
        fmt::println("\tflags_live_in: {}", formatFlagsMask(blockLiveness.flagsLiveIn));
        fmt::println("\tflags_live_out: {}", formatFlagsMask(blockLiveness.flagsLiveOut));

        blockJson["Instr Liveness"] = std::move(instrJson);

        json[fmt::format("{:#x}", address)] = std::move(blockJson);
    }

    fmt::println("{}", script);

    auto toHex = [](uint64_t value)
    {
        char buffer[64] = "";
        sprintf_s(buffer, "\"0x%llX\"", value);
        return std::string(buffer);
    };

    std::string dot = "digraph G {\n";
    for (const auto& [address, block] : cfg.blocks)
    {
        dot += toHex(address) + " [label=\"" + cfg.program.getLabelData(block.label).value().name + "\"];\n";
        for (const auto& successor : block.successors)
        {
            auto data = cfg.program.getLabelData(successor).value().node->getUserData<InstructionData>();
            auto successorAddress = data->address;
            dot += toHex(address) + " -> " + toHex(successorAddress) + ";\n";
        }
    }
    dot += "}";

    fmt::println("{}", dot);

    std::ofstream ofs("liveness.json");
    ofs << json.dump(2);
}

int main(int argc, char** argv)
{
    Arguments args(argc, argv);
    auto      verbose = args.verbose;

    std::vector<uint8_t> pe;
    if (!loadFile(args.input, pe))
    {
        fmt::println("Failed to load the executable.");
        return EXIT_FAILURE;
    }

    uint64_t             riscvmRunAddress = 0;
    std::vector<uint8_t> riscvmRunCode;
    if (!findFunction(pe, "riscvm_run", riscvmRunAddress, riscvmRunCode))
    {
        fmt::println("Failed to find riscvm_run function.");
        return EXIT_FAILURE;
    }

    fmt::println("riscvm_run address: {:#x}, size: {:#x}", riscvmRunAddress, riscvmRunCode.size());

    ObfuscationProgram program(MachineMode::AMD64);
    if (!disassemble(program, "riscvm_run", riscvmRunAddress, riscvmRunCode, verbose))
    {
        fmt::println("Failed to disassemble riscvm_run function.");
        return EXIT_FAILURE;
    }

    // Analyze the CFG
    auto cfg = CFG::analyze(program, program.getEntryPoint(), verbose);
    if (!cfg)
    {
        fmt::println("Failed to analyze the riscvm_run function: {}", cfg.error());
        return EXIT_FAILURE;
    }

    // Perform liveness analysis
    auto livenessBlocks      = cfg->getLivenessBlocks(verbose);
    auto instructionLiveness = cfg->getInstructionLiveness(livenessBlocks, verbose);

    // Add liveness information to the instruction data
    for (const auto& instruction : instructionLiveness)
    {
        auto data       = instruction.node->getUserData<InstructionData>();
        data->regsLive  = instruction.regsLive;
        data->flagsLive = instruction.flagsLive;
    }

    if (verbose)
    {
        dumpLiveness(*cfg, livenessBlocks);
    }

    auto serializeToFile = [&program](const std::string& outputFile, uint64_t base = 0)
    {
        // Serialize the obfuscated function
        Serializer serializer;
        if (auto res = serializer.serialize(program, base); res != zasm::ErrorCode::None)
        {
            fmt::println("Failed to serialize program at {:#x}, {}", base, res.getErrorName());
            return false;
        }

        auto ptr  = serializer.getCode();
        auto size = serializer.getCodeSize();

        // Save the code to disk
        std::ofstream ofs(outputFile, std::ios::binary);
        ofs.write((char*)ptr, size);
        return true;
    };

    if (!args.cleanOutput.empty() && !serializeToFile(args.cleanOutput))
    {
        return EXIT_FAILURE;
    }

    if (!obfuscate(program, verbose))
    {
        fmt::println("Failed to obfuscate riscvm_run function.");
        return EXIT_FAILURE;
    }

    fmt::println("\n{}", formatter::toString(program));

    if (!args.output.empty() && !serializeToFile(args.output))
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
