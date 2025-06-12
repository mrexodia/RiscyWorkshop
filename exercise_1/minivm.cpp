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
    const uint8_t*  bytecode;
    const uint32_t* labels;
    uint64_t        pc;
    uint64_t        regs[256];

    ALWAYS_INLINE uint8_t fetch()
    {
        return bytecode[pc++];
    }

    ALWAYS_INLINE uint64_t& op_reg()
    {
        return regs[fetch()];
    }

    ALWAYS_INLINE uint64_t op_imm64()
    {
        auto imm = *(uint64_t*)&bytecode[pc];
        pc += sizeof(uint64_t);
        return imm;
    }
};

template <size_t MaxLabels = 5> struct VMLabels
{
    template <size_t Size> constexpr VMLabels(const uint8_t (&bytecode)[Size]) : labels()
    {
        for (size_t i = 0; i < std::size(labels); i++)
        {
            labels[i] = -1;
        }

        for (size_t i = 0; i + 10 < Size; i++)
        {
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
};

using VMHandler = uint64_t (*)(VMContext& ctx);

static uint64_t handler_label(VMContext& ctx);
static uint64_t handler_ret(VMContext& ctx);
static uint64_t handler_add(VMContext& ctx);
static uint64_t handler_movimm(VMContext& ctx);
static uint64_t handler_cmp(VMContext& ctx);
static uint64_t handler_jcc(VMContext& ctx);
static uint64_t handler_xor(VMContext& ctx);
static uint64_t handler_or(VMContext& ctx);
static uint64_t handler_mul(VMContext& ctx);

static VMHandler handlers[] = {
    handler_label,
    handler_ret,
    handler_add,
    handler_movimm,
    handler_cmp,
    handler_jcc,
    handler_xor,
    handler_or,
    handler_mul,
};

#ifdef VMDEBUG
#define dispatch(ctx)                                           \
    {                                                           \
        auto check = ctx.fetch();                               \
        if (check != 0xFF)                                      \
        {                                                       \
            printf("OPCODE MISALIGNED AT %zu\n", ctx.pc - 1);   \
            return -1;                                          \
        }                                                       \
        auto opcode = ctx.fetch();                              \
        __attribute__((musttail)) return handlers[opcode](ctx); \
    }
#else
#define dispatch(ctx) __attribute__((musttail)) return handlers[ctx.bytecode[ctx.pc++]](ctx)
#endif // VMDEBUG

static uint64_t handler_label(VMContext& ctx)
{
    ctx.pc += 9;
    dispatch(ctx);
}

static uint64_t handler_ret(VMContext& ctx)
{
    return ctx.op_reg();
}

static uint64_t handler_add(VMContext& ctx)
{
    auto& dst = ctx.op_reg();
    auto& op1 = ctx.op_reg();
    auto& op2 = ctx.op_reg();
    dst       = op1 + op2;
    dispatch(ctx);
}

static uint64_t handler_movimm(VMContext& ctx)
{
    auto& dst = ctx.op_reg();
    dst       = ctx.op_imm64();
    dispatch(ctx);
}

static uint64_t handler_cmp(VMContext& ctx)
{
    auto& dst = ctx.op_reg();
    auto& op1 = ctx.op_reg();
    auto& op2 = ctx.op_reg();
    dst       = op1 == op2;
    dispatch(ctx);
}

static uint64_t handler_jcc(VMContext& ctx)
{
    auto& cond  = ctx.op_reg();
    auto  label = ctx.fetch();
    if (cond)
    {
        ctx.pc = ctx.labels[label];
    }
    dispatch(ctx);
}

static uint64_t handler_xor(VMContext& ctx)
{
    auto& dst = ctx.op_reg();
    auto& op1 = ctx.op_reg();
    auto& op2 = ctx.op_reg();
    dst       = op1 ^ op2;
    dispatch(ctx);
}

static uint64_t handler_or(VMContext& ctx)
{
    auto& dst = ctx.op_reg();
    auto& op1 = ctx.op_reg();
    auto& op2 = ctx.op_reg();
    dst       = op1 | op2;
    dispatch(ctx);
}

static uint64_t handler_mul(VMContext& ctx)
{
    auto& dst = ctx.op_reg();
    auto& op1 = ctx.op_reg();
    auto& op2 = ctx.op_reg();
    dst       = op1 * op2;
    dispatch(ctx);
}

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

#define LABEL(index)       OPCODE(0), 0x12, 0x34, 0x56, 0x78, index, 0x87, 0x65, 0x43, 0x21
#define RET(op)            OPCODE(1), op
#define ADD(dst, op1, op2) OPCODE(2), dst, op1, op2
#define MOVIMM(dst, imm64) OPCODE(3), dst, IMM64(imm64)
#define CMP(dst, op1, op2) OPCODE(4), dst, op1, op2
#define JCC(cond, label)   OPCODE(5), cond, label
#define XOR(dst, op1, op2) OPCODE(6), dst, op1, op2
#define OR(dst, op1, op2)  OPCODE(7), dst, op1, op2

/*
constexpr uint8_t bytecode1[] =
{
    MOVIMM(REG(254), 0x2),
    CMP(REG(255), REG(0), REG(254)),
    JCC(REG(255), 0),
    RET(REG(254)),
    LABEL(0),
    ADD(REG(0), REG(0), REG(1)),
    MOVIMM(REG(0), 0x1122334455667788),
    RET(REG(0)),
};
*/

constexpr uint8_t bytecode1[] = {
    OR(REG(4), REG(0), REG(1)),
    XOR(REG(5), REG(2), REG(3)),
    ADD(REG(6), REG(4), REG(5)),
    RET(REG(6)),
};

constexpr static VMLabels labels1 = VMLabels(bytecode1);

// This makes sure clang -O2 doesn't optimize the whole VM away
const uint8_t* bytecode1_ptr = bytecode1;

static __attribute__((optnone)) uint64_t execute_bytecode(
    const uint8_t* bytecode, const uint32_t* labels, uint64_t r0, uint64_t r1, uint64_t r2, uint64_t r3
)
{
    VMContext ctx;
    ctx.bytecode = bytecode;
    ctx.labels   = labels;
    ctx.pc       = 0;
    ctx.regs[0]  = r0;
    ctx.regs[1]  = r1;
    ctx.regs[2]  = r2;
    ctx.regs[3]  = r3;
#ifdef VMDEBUG
    ctx.pc++;
#endif
    return handlers[ctx.fetch()](ctx);
}

extern __attribute__((noinline)) uint64_t vm_bytecode1(uint64_t r0, uint64_t r1, uint64_t r2, uint64_t r3)
{
    return execute_bytecode(bytecode1, labels1.labels, r0, r1, r2, r3);
}

uint64_t test(const char* a, const char* b, const char* c, const char* d)
{
    return vm_bytecode1(atoi(a), atoi(b), atoi(c), atoi(d));
}

int main(int argc, char** argv)
{
    if (argc < 5)
    {
        puts("Usage: minivm a b c d");
        return 1;
    }
    auto ret = test(argv[1], argv[2], argv[3], argv[4]);
    printf("result: %" PRIu64 "\n", ret);
}
