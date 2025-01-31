#pragma once

#include <obfuscator/program.hpp>

namespace obfuscator
{
bool disassemble(ObfuscationProgram& program, const char* name, const uint64_t functionStart, const std::vector<uint8_t>& code, bool verbose = false);
}
