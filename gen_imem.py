prog = [
    # --- operand setup ---
    ("addi", "x1", "x0", 10),
    ("addi", "x2", "x0", 3),
    ("addi", "x3", "x0", -8),
    # --- R-type (10) ---
    ("add",  "x5",  "x1", "x2"),
    ("sub",  "x6",  "x1", "x2"),
    ("sll",  "x7",  "x1", "x2"),
    ("slt",  "x8",  "x3", "x1"),
    ("sltu", "x9",  "x3", "x1"),
    ("xor",  "x10", "x1", "x2"),
    ("srl",  "x11", "x1", "x2"),
    ("sra",  "x12", "x3", "x2"),
    ("or",   "x13", "x1", "x2"),
    ("and",  "x14", "x1", "x2"),
    # --- I-arith (9) ---
    ("addi",  "x15", "x1", 5),
    ("slti",  "x16", "x3", 0),
    ("sltiu", "x17", "x1", 20),
    ("xori",  "x18", "x1", 6),
    ("ori",   "x19", "x1", 5),
    ("andi",  "x20", "x1", 6),
    ("slli",  "x21", "x1", 2),
    ("srli",  "x22", "x1", 1),
    ("srai",  "x23", "x3", 1),
    # --- U-type (2) ---
    ("lui",   "x24", 0x12345),
    ("auipc", "x25", 0x0),
    # --- store / load (8), base x4 = 0 ---
    ("addi", "x4", "x0", 0),
    ("sw",  "x5",  0,  "x4"),
    ("lw",  "x26", 0,  "x4"),
    ("sh",  "x21", 8,  "x4"),
    ("lh",  "x29", 8,  "x4"),
    ("lhu", "x30", 8,  "x4"),
    ("sb",  "x3",  12, "x4"),
    ("lb",  "x27", 12, "x4"),
    ("lbu", "x28", 12, "x4"),
    # --- branches (6), marker = x31 ---
    ("addi", "x31", "x0", 0),
    ("beq",  "x1", "x1", 8),
    ("addi", "x31", "x31", 100),   # POISON
    ("addi", "x31", "x31", 1),     # ->1
    ("bne",  "x1", "x2", 8),
    ("addi", "x31", "x31", 100),   # POISON
    ("addi", "x31", "x31", 1),     # ->2
    ("blt",  "x3", "x1", 8),
    ("addi", "x31", "x31", 100),   # POISON
    ("addi", "x31", "x31", 1),     # ->3
    ("bge",  "x1", "x3", 8),
    ("addi", "x31", "x31", 100),   # POISON
    ("addi", "x31", "x31", 1),     # ->4
    ("bltu", "x1", "x3", 8),
    ("addi", "x31", "x31", 100),   # POISON
    ("addi", "x31", "x31", 1),     # ->5
    ("bgeu", "x3", "x1", 8),
    ("addi", "x31", "x31", 100),   # POISON
    ("addi", "x31", "x31", 1),     # ->6
    # --- jumps (2) ---
    ("jal",  "x4", 8),
    ("addi", "x31", "x31", 100),   # POISON
    ("addi", "x31", "x31", 1),     # ->7
    ("auipc", "x4", 0x0),
    ("jalr", "x4", "x4", 12),
    ("addi", "x31", "x31", 100),   # POISON
    ("addi", "x31", "x31", 1),     # ->8
    # --- halt ---
    ("beq", "x0", "x0", 0),        # infinite self-loop
]

# ---------------- encoding helpers ----------------
def reg(r):
    assert r[0] == 'x', f"bad register {r}"
    n = int(r[1:])
    assert 0 <= n < 32, f"reg out of range {r}"
    return n

def u2(val, bits):
    """two's-complement mask to 'bits' width"""
    return val & ((1 << bits) - 1)

# opcode/funct tables
R = {  # mnemonic: (funct7, funct3)
    "add": (0b0000000, 0b000), "sub": (0b0100000, 0b000),
    "sll": (0b0000000, 0b001), "slt": (0b0000000, 0b010),
    "sltu":(0b0000000, 0b011), "xor": (0b0000000, 0b100),
    "srl": (0b0000000, 0b101), "sra": (0b0100000, 0b101),
    "or":  (0b0000000, 0b110), "and": (0b0000000, 0b111),
}
I = {  # mnemonic: funct3   (opcode 0010011)
    "addi": 0b000, "slti": 0b010, "sltiu": 0b011, "xori": 0b100,
    "ori": 0b110, "andi": 0b111,
}
SH = {  # shift-immediate: funct3, funct7
    "slli": (0b001, 0b0000000), "srli": (0b101, 0b0000000),
    "srai": (0b101, 0b0100000),
}
LOAD = {"lb": 0b000, "lh": 0b001, "lw": 0b010, "lbu": 0b100, "lhu": 0b101}
STORE = {"sb": 0b000, "sh": 0b001, "sw": 0b010}
BR = {"beq": 0b000, "bne": 0b001, "blt": 0b100, "bge": 0b101,
      "bltu": 0b110, "bgeu": 0b111}

def enc_r(rd, rs1, rs2, funct7, funct3):
    return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | 0b0110011

def enc_i(rd, rs1, imm, funct3, opcode):
    return (u2(imm, 12) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode

def enc_sh(rd, rs1, shamt, funct3, funct7):
    return (funct7 << 25) | (u2(shamt, 5) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | 0b0010011

def enc_s(rs2, rs1, imm, funct3):
    imm = u2(imm, 12)
    hi = (imm >> 5) & 0x7f
    lo = imm & 0x1f
    return (hi << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (lo << 7) | 0b0100011

def enc_b(rs1, rs2, imm, funct3):
    imm = u2(imm, 13)  # imm[12:0], bit0 = 0
    b12 = (imm >> 12) & 1
    b11 = (imm >> 11) & 1
    b10_5 = (imm >> 5) & 0x3f
    b4_1 = (imm >> 1) & 0xf
    return (b12 << 31) | (b10_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (b4_1 << 8) | (b11 << 7) | 0b1100011

def enc_u(rd, imm20, opcode):
    return ((imm20 & 0xfffff) << 12) | (rd << 7) | opcode

def enc_j(rd, imm):
    imm = u2(imm, 21)  # imm[20:0], bit0 = 0
    b20 = (imm >> 20) & 1
    b19_12 = (imm >> 12) & 0xff
    b11 = (imm >> 11) & 1
    b10_1 = (imm >> 1) & 0x3ff
    return (b20 << 31) | (b10_1 << 21) | (b11 << 20) | (b19_12 << 12) | (rd << 7) | 0b1101111

def encode(instr):
    op = instr[0]
    if op in R:
        f7, f3 = R[op]
        return enc_r(reg(instr[1]), reg(instr[2]), reg(instr[3]), f7, f3)
    if op in I:
        return enc_i(reg(instr[1]), reg(instr[2]), instr[3], I[op], 0b0010011)
    if op in SH:
        f3, f7 = SH[op]
        return enc_sh(reg(instr[1]), reg(instr[2]), instr[3], f3, f7)
    if op in LOAD:
        # (op, rd, offset, base)
        return enc_i(reg(instr[1]), reg(instr[3]), instr[2], LOAD[op], 0b0000011)
    if op in STORE:
        # (op, rs2, offset, base)
        return enc_s(reg(instr[1]), reg(instr[3]), instr[2], STORE[op])
    if op in BR:
        return enc_b(reg(instr[1]), reg(instr[2]), instr[3], BR[op])
    if op == "lui":
        return enc_u(reg(instr[1]), instr[2], 0b0110111)
    if op == "auipc":
        return enc_u(reg(instr[1]), instr[2], 0b0010111)
    if op == "jal":
        return enc_j(reg(instr[1]), instr[2])
    if op == "jalr":
        return enc_i(reg(instr[1]), reg(instr[2]), instr[3], 0b000, 0b1100111)
    raise ValueError(f"unknown instruction: {op}")

# ---------------- main ----------------
lines = []
for idx, instr in enumerate(prog):
    word = encode(instr) & 0xffffffff
    bits = format(word, "032b")
    lines.append(bits)
    asm = instr[0] + " " + ", ".join(str(a) for a in instr[1:])
    print(f"{idx:02d}  0x{word:08x}  {bits}  # {asm}")

with open("imem.mem", "w", newline="\n") as f:
    f.write("\n".join(lines) + "\n")

print(f"\nWrote imem.mem ({len(lines)} instructions).")
