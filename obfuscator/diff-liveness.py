import json
import argparse

def liveness_eq(title: str, a: dict[str, list[str]], b: dict[str, list[str]]) -> bool:
    aflags = set(a["flags"])
    bflags = set(b["flags"])
    if aflags != bflags:
        print(f"{title} flags differ")
        print(f"  A: {sorted(aflags)}")
        print(f"  B: {sorted(bflags)}")
    aregs = set(a["regs"])
    bregs = set(b["regs"])
    if aregs != bregs:
        print(f"{title} regs differ")
        print(f"  A: {sorted(aregs)}")
        print(f"  B: {sorted(bregs)}")
    return True

def main():
    parser = argparse.ArgumentParser(description="Diff liveness")
    parser.add_argument("A", type=str, help="Old liveness")
    parser.add_argument("B", type=str, help="New liveness")
    args = parser.parse_args()

    old = json.load(open(args.A))
    new = json.load(open(args.B))

    for block_addr in old:
        if block_addr not in new:
            print(f"Block {block_addr} not found in new")
            continue
        old_block = old[block_addr]
        new_block = new[block_addr]
        liveness_eq(f"Block {block_addr} liveness in", old_block["Liveness in"], new_block["Liveness in"])
        liveness_eq(f"Block {block_addr} liveness out", old_block["Liveness out"], new_block["Liveness out"])
        for instr_addr in old_block["Instr Liveness"]:
            if instr_addr not in new_block["Instr Liveness"]:
                print(f"Instr {instr_addr} not found in new")
                continue
            old_instr = old_block["Instr Liveness"][instr_addr]
            new_instr = new_block["Instr Liveness"][instr_addr]
            liveness_eq(f"Block {block_addr} -> {instr_addr}", old_instr, new_instr)

if __name__ == "__main__":
    main()
