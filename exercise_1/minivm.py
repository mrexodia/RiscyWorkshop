#!/usr/bin/env python3

import sys
import struct
import argparse

class VMContext:
    def __init__(self, bytecode, labels):
        self.bytecode = bytecode
        self.labels = labels
        self.pc = 0
        self.regs = [0] * 256

    def fetch(self):
        """Fetches the next byte from the bytecode and increments the program counter"""
        data = self.bytecode[self.pc]
        self.pc += 1
        return data

    def op_reg(self):
        """Fetches register index"""
        reg_idx = self.fetch()
        return reg_idx

    def op_imm64(self):
        """Fetch 64-bit immediate value (little-endian)"""
        imm_bytes = self.bytecode[self.pc:self.pc + 8]
        self.pc += 8
        return struct.unpack('<q', bytes(imm_bytes))[0]

# NOTE: You do not need to understand this for the exercises
class VMPreprocessor:
    """Preprocesses bytecode to find label positions"""

    def __init__(self, bytecode: bytes):
        self.data = list(bytecode)
        self.size = len(bytecode)
        self.labels = [-1] * 5  # Support up to 5 labels

        # Find label placeholders in bytecode
        # Pattern: 0x00, 0x12, 0x34, 0x56, 0x78, index, 0x87, 0x65, 0x43, 0x21
        for i in range(len(bytecode) - 9):
            if (bytecode[i] == 0x00 and
                bytecode[i + 1] == 0x12 and
                bytecode[i + 2] == 0x34 and
                bytecode[i + 3] == 0x56 and
                bytecode[i + 4] == 0x78 and
                bytecode[i + 6] == 0x87 and
                bytecode[i + 7] == 0x65 and
                bytecode[i + 8] == 0x43 and
                bytecode[i + 9] == 0x21):

                index = bytecode[i + 5]
                if index < len(self.labels):
                    if self.labels[index] == -1:
                        self.labels[index] = i
                    else:
                        raise ValueError("Duplicate LABEL_PLACEHOLDER detected")

def handler_label(ctx: VMContext):
    ctx.pc += 9

def handler_ret(ctx: VMContext):
    reg_idx = ctx.op_reg()
    return ctx.regs[reg_idx]

def handler_add(ctx: VMContext):
    dst = ctx.op_reg()
    op1 = ctx.op_reg()
    op2 = ctx.op_reg()
    ctx.regs[dst] = (ctx.regs[op1] + ctx.regs[op2]) & 0xFFFFFFFFFFFFFFFF

def handler_movimm(ctx: VMContext):
    dst = ctx.op_reg()
    ctx.regs[dst] = ctx.op_imm64()

def handler_cmp(ctx: VMContext):
    dst = ctx.op_reg()
    op1 = ctx.op_reg()
    op2 = ctx.op_reg()
    ctx.regs[dst] = 1 if ctx.regs[op1] == ctx.regs[op2] else 0

def handler_jcc(ctx: VMContext):
    cond_reg = ctx.op_reg()
    label = ctx.fetch()
    if ctx.regs[cond_reg] != 0:
        ctx.pc = ctx.labels[label]

def handler_xor(ctx: VMContext):
    dst = ctx.op_reg()
    op1 = ctx.op_reg()
    op2 = ctx.op_reg()
    ctx.regs[dst] = ctx.regs[op1] ^ ctx.regs[op2]

def handler_or(ctx: VMContext):
    dst = ctx.op_reg()
    op1 = ctx.op_reg()
    op2 = ctx.op_reg()
    ctx.regs[dst] = ctx.regs[op1] | ctx.regs[op2]

def handler_mul(ctx: VMContext):
    dst = ctx.op_reg()
    op1 = ctx.op_reg()
    op2 = ctx.op_reg()
    ctx.regs[dst] = (ctx.regs[op1] * ctx.regs[op2]) & 0xFFFFFFFFFFFFFFFF

def execute_bytecode(preprocessor: VMPreprocessor, r0: int, r1: int, r2: int, r3: int) -> int:
    ctx = VMContext(preprocessor.data, preprocessor.labels)
    ctx.regs[0] = r0
    ctx.regs[1] = r1
    ctx.regs[2] = r2
    ctx.regs[3] = r3

    while True:
        # Get instruction opcode
        opcode = ctx.fetch()
        match opcode:
            case 0:
                handler_label(ctx)
            case 1:
                return handler_ret(ctx)
            case 2:
                handler_add(ctx)
            case 3:
                handler_movimm(ctx)
            case 4:
                handler_cmp(ctx)
            case 5:
                handler_jcc(ctx)
            case 6:
                handler_xor(ctx)
            case 7:
                handler_or(ctx)
            case 8:
                handler_mul(ctx)
            case _:
                raise ValueError(f"Invalid opcode: {opcode:02X}")

def main():
    # Parse arguments
    parser = argparse.ArgumentParser(description="Mini Virtual Machine - Python Port")
    parser.add_argument("--bytecode", type=str, help="Hex string of bytecode", default="07 04 00 01 06 05 02 03 02 06 04 05 01 06")
    parser.add_argument("inputs", nargs="*", type=int, help="Initial register values (max 4)")
    args = parser.parse_args()

    inputs = [0, 0, 0, 0]
    for i, arg in enumerate(args.inputs[:4]):
        inputs[i] = arg
    bytecode = bytes.fromhex(args.bytecode)

    # Display inputs
    print(f"arguments: ({inputs[0]}, {inputs[1]}, {inputs[2]}, {inputs[3]})")
    print(f"data: {bytecode.hex().upper()}")

    # Preprocess and execute
    preprocessor = VMPreprocessor(bytecode)
    result = execute_bytecode(preprocessor, inputs[0], inputs[1], inputs[2], inputs[3])
    result = struct.unpack('<q', struct.pack('<Q', result))[0]
    print(f"result: {result}")

if __name__ == "__main__":
    sys.exit(main())
