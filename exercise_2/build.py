#!/usr/bin/env python3

import subprocess
import argparse
import shlex
import sys
import os

def cmd(command, *args):
    final = [command] + list(args)
    print("$", shlex.join(final))
    result = subprocess.run(final, capture_output=True, text=True, encoding="utf-8", errors="ignore")
    output = result.stdout.strip()
    if len(output) == 0:
        output = result.stderr.strip()
    else:
        output += "\n"
        output += result.stderr
    if result.returncode != 0:
        print(f"\n=== Command failed with status {result.returncode} ===\n{output}")
        sys.exit(1)
    return output

def main():
    parser = argparse.ArgumentParser("Shellcode builder")
    parser.add_argument("c_file", help="Path to the C file to compile to shellcode")
    # Bonus: add a --disassemble flag to automatically disassemble the shellcode
    # Bonus: add a --run flag to automatically run the shellcode
    args = parser.parse_args()

    c_file = args.c_file
    if not os.path.exists(c_file):
        raise FileNotFoundError(f"File not found: {c_file}")

    basename = os.path.basename(c_file)
    file, ext = os.path.splitext(basename)

    print(file)
    print(ext)

    obj = f"{file}.o"
    cmd("clang-20", "-target", "riscv64", "-march=rv64im", "-mcmodel=medany", "-Os", "-c", c_file, "-o", obj)
    # TODO: implement the rest of the build pipeline

if __name__ == "__main__":
    main()
