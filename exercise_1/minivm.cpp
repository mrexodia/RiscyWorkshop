/*
#!/bin/sh
clang -O3 -fno-slp-vectorize -std=c++17 minivm.cpp -o minivm-macho
zig c++ -target x86_64-linux -std=c++20 -O3 -fno-slp-vectorize minivm.cpp -o minivm-elf
zig c++ -target x86_64-windows -std=c++20 -O3 -fno-slp-vectorize minivm.cpp -o minivm-windows
*/
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cinttypes>
#include <iterator>

#define VMDEBUG 0

#if defined(VMDEBUG) && VMDEBUG == 0
#undef VMDEBUG
#endif // VMDEBUG

#define ALWAYS_INLINE __attribute__((always_inline))

struct VMContext
{
    const uint8_t*  bytecode  = nullptr;
    const uint32_t* labels    = nullptr;
    uint64_t        pc        = 0;
    uint64_t        regs[256] = {};

    // Fetches the next byte from the bytecode and increments the program counter
    ALWAYS_INLINE uint8_t fetch()
    {
        uint8_t data = bytecode[pc];
        pc += 1;
        return data;
    }

    // Fetches the register index from the bytecode and returns a reference to the
    // corresponding register
    ALWAYS_INLINE uint64_t& op_reg()
    {
        auto reg_idx = fetch();
        return regs[reg_idx];
    }

    // Fetches a 64-bit immediate value from the bytecode and returns it
    // increments the program counter
    ALWAYS_INLINE uint64_t op_imm64()
    {
        auto imm = *(uint64_t*)&bytecode[pc];
        pc += sizeof(uint64_t);
        return imm;
    }
};

template <size_t MaxLabels = 5, size_t MaxBytecode = 1024> struct VMBytecode
{
    template <size_t Size> constexpr VMBytecode(const uint8_t (&bytecode)[Size]) : labels()
    {
        // Initialize the labels to -1
        for (size_t i = 0; i < std::size(labels); i++)
        {
            labels[i] = -1;
        }

        for (size_t i = 0; i + 10 < Size; i++)
        {
            data[i] = bytecode[i];
            if (bytecode[i] == 0 && bytecode[i + 1] == 0x12 && bytecode[i + 2] == 0x34
                && bytecode[i + 3] == 0x56 && bytecode[i + 4] == 0x78 && bytecode[i + 6] == 0x87
                && bytecode[i + 7] == 0x65 && bytecode[i + 8] == 0x43 && bytecode[i + 9] == 0x21)
            {
                auto index = bytecode[i + 5];
                if (labels[index] == -1)
                {
#ifdef VMDEBUG
                    labels[index] = i - 1;
#else
                    labels[index] = i;
#endif // VMDEBUG
                }
                else
                {
                    labels[index] = -2;
                }
            }
        }
    }

    uint32_t labels[MaxLabels];
    uint8_t  data[MaxBytecode] = {};
};

static ALWAYS_INLINE void handler_label(VMContext& ctx)
{
    // The label bytecode contains a placeholder for the compile-time processing
    // happening in the VMLabels constructor. This is why we need to skip 9 bytes:
    // 0x12, 0x34, 0x56, 0x78, index, 0x87, 0x65, 0x43, 0x21
    ctx.pc += 9;
}

static ALWAYS_INLINE uint64_t handler_ret(VMContext& ctx)
{
    return ctx.op_reg();
}

static ALWAYS_INLINE void handler_add(VMContext& ctx)
{
    auto& dst = ctx.op_reg();
    auto& op1 = ctx.op_reg();
    auto& op2 = ctx.op_reg();
    dst       = op1 + op2;
}

static ALWAYS_INLINE void handler_movimm(VMContext& ctx)
{
    auto& dst = ctx.op_reg();
    dst       = ctx.op_imm64();
}

static ALWAYS_INLINE void handler_cmp(VMContext& ctx)
{
    auto& dst = ctx.op_reg();
    auto& op1 = ctx.op_reg();
    auto& op2 = ctx.op_reg();
    dst       = op1 == op2;
}

static ALWAYS_INLINE void handler_jcc(VMContext& ctx)
{
    auto& cond  = ctx.op_reg();
    auto  label = ctx.fetch();
    if (cond)
    {
        ctx.pc = ctx.labels[label];
    }
}

static ALWAYS_INLINE void handler_xor(VMContext& ctx)
{
    auto& dst = ctx.op_reg();
    auto& op1 = ctx.op_reg();
    auto& op2 = ctx.op_reg();
    dst       = op1 ^ op2;
}

static ALWAYS_INLINE void handler_or(VMContext& ctx)
{
    auto& dst = ctx.op_reg();
    auto& op1 = ctx.op_reg();
    auto& op2 = ctx.op_reg();
    dst       = op1 | op2;
}

static ALWAYS_INLINE void handler_mul(VMContext& ctx)
{
    auto& dst = ctx.op_reg();
    auto& op1 = ctx.op_reg();
    auto& op2 = ctx.op_reg();
    dst       = op1 * op2;
}

static __attribute__((noinline)) uint64_t
execute_bytecode(const VMBytecode<>& bytecode, uint64_t r0, uint64_t r1, uint64_t r2, uint64_t r3)
{
    VMContext ctx;
    ctx.bytecode = bytecode.data;
    ctx.labels   = bytecode.labels;
    ctx.pc       = 0;
    ctx.regs[0]  = r0;
    ctx.regs[1]  = r1;
    ctx.regs[2]  = r2;
    ctx.regs[3]  = r3;
#ifdef VMDEBUG
    ctx.pc++;
#endif

    while (true)
    {
        // Get instruction opcode
        uint8_t opcode = ctx.fetch();
        switch (opcode)
        {
        case 0:
            handler_label(ctx);
            break;
        case 1:
            return handler_ret(ctx);
        case 2:
            handler_add(ctx);
            break;
        case 3:
            handler_movimm(ctx);
            break;
        case 4:
            handler_cmp(ctx);
            break;
        case 5:
            handler_jcc(ctx);
            break;
        case 6:
            handler_xor(ctx);
            break;
        case 7:
            handler_or(ctx);
            break;
        case 8:
            handler_mul(ctx);
            break;
        default:
            __builtin_unreachable();
        }
    }
}

// Helpers to make it easier to write VM bytecode
#define REG(n) n

#if defined(__LITTLE_ENDIAN__)
#define EXTRACT(imm64, byte) (((uint64_t)imm64 >> (8 * (byte))) & 0xFF)
#elif defined(__BIG_ENDIAN__)
#define EXTRACT(imm64, byte) (((uint64_t)imm64 >> (8 * (7 - byte))) & 0xFF)
#else
#error "Failed to detect endianness"
#endif

#ifdef VMDEBUG
#define OPCODE(index) 0xFF, index
#else
#define OPCODE(index) index
#endif // VMDEBUG
#define IMM64(imm64)                                                                               \
    EXTRACT(imm64, 0), EXTRACT(imm64, 1), EXTRACT(imm64, 2), EXTRACT(imm64, 3), EXTRACT(imm64, 4), \
        EXTRACT(imm64, 5), EXTRACT(imm64, 6), EXTRACT(imm64, 7)

#define LABEL_PLACEHOLDER(index) OPCODE(0), 0x12, 0x34, 0x56, 0x78, index, 0x87, 0x65, 0x43, 0x21
#define RET(op)                  OPCODE(1), op
#define ADD(dst, op1, op2)       OPCODE(2), dst, op1, op2
#define MOVIMM(dst, imm64)       OPCODE(3), dst, IMM64(imm64)
#define CMP(dst, op1, op2)       OPCODE(4), dst, op1, op2
#define JCC(cond, label)         OPCODE(5), cond, label
#define XOR(dst, op1, op2)       OPCODE(6), dst, op1, op2
#define OR(dst, op1, op2)        OPCODE(7), dst, op1, op2
#define MUL(dst, op1, op2)       OPCODE(8), dst, op1, op2

/*
constexpr static VMBytecode bytecode = VMBytecode({
    MOVIMM(REG(254), 0x2),
    CMP(REG(255), REG(0), REG(254)),
    JCC(REG(255), 0), // jumps to LABEL_PLACEHOLDER(0) if REG(255) != 0 (so REG(0) == 2)
    RET(REG(254)),
    LABEL_PLACEHOLDER(0),
    ADD(REG(0), REG(0), REG(1)),
    MOVIMM(REG(0), 0x1122334455667788),
    RET(REG(0)),
};
*/

constexpr static VMBytecode bytecode = VMBytecode({
    OR(REG(4), REG(0), REG(1)),  // r4 = r0 | r1
    XOR(REG(5), REG(2), REG(3)), // r5 = r2 ^ r3
    ADD(REG(6), REG(4), REG(5)), // r6 = r4 + r5
    RET(REG(6)),                 // return r6
});

int main(int argc, char** argv)
{
    // Usage: ./minivm 1 2 3 4
    // Missing arguments default to zero
    uint64_t args[4] = {};
    for (int i = 1; i < argc; i++)
    {
        args[i - 1] = atoi(argv[i]);
    }

    printf("arguments: (%" PRIi64 ", %" PRIi64 ", %" PRIi64 ", %" PRIi64 ")\n", args[0], args[1], args[2], args[3]);
    auto ret = execute_bytecode(bytecode, args[0], args[1], args[2], args[3]);
    printf("result: %" PRIi64 "\n", ret);
}
