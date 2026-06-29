# --- operand setup ---
00  addi x1, x0, 10            # x1 = 10
01  addi x2, x0, 3             # x2 = 3
02  addi x3, x0, -8            # x3 = 0xFFFFFFF8  (negative: for SLT/SRA/LB sign tests)
# --- R-type (10) ---
03  add  x5,  x1, x2           # 13
04  sub  x6,  x1, x2           # 7
05  sll  x7,  x1, x2           # 80   (10<<3)
06  slt  x8,  x3, x1           # 1    (-8 < 10, signed)
07  sltu x9,  x3, x1           # 0    (0xFFFFFFF8 < 10 unsigned? no)
08  xor  x10, x1, x2           # 9
09  srl  x11, x1, x2           # 1    (10>>3 logical)
10  sra  x12, x3, x2           # 0xFFFFFFFF  (-8>>>3 = -1, arithmetic)
11  or   x13, x1, x2           # 11
12  and  x14, x1, x2           # 2
# --- I-arith (9) ---
13  addi  x15, x1, 5           # 15
14  slti  x16, x3, 0           # 1    (-8 < 0)
15  sltiu x17, x1, 20          # 1
16  xori  x18, x1, 6           # 12
17  ori   x19, x1, 5           # 15
18  andi  x20, x1, 6           # 2
19  slli  x21, x1, 2           # 40   (10<<2)
20  srli  x22, x1, 1           # 5    (10>>1 logical)
21  srai  x23, x3, 1           # 0xFFFFFFFC  (-4, arithmetic)
# --- U-type (2) ---
22  lui   x24, 0x12345         # 0x12345000
23  auipc x25, 0x0             # 0x0000005C  (PC of this instr = 23*4 = 92)
# --- store / load (8), base x4 = 0 ---
24  addi x4, x0, 0             # base = 0
25  sw   x5,  0(x4)            # mem[0]  = 13
26  lw   x26, 0(x4)            # 13
27  sh   x21, 8(x4)            # mem[8]  = 40
28  lh   x29, 8(x4)            # 40
29  lhu  x30, 8(x4)            # 40
30  sb   x3,  12(x4)           # mem[12] byte = 0xF8  (low byte of -8)
31  lb   x27, 12(x4)           # 0xFFFFFFF8  (sign-extended)
32  lbu  x28, 12(x4)           # 0x000000F8  (zero-extended)
# --- branches (6), marker = x31 ---
33  addi x31, x0, 0            # marker = 0
34  beq  x1, x1, 8            # equal -> jump, skip poison
35  addi x31, x31, 100         #   <-- POISON (must be skipped)
36  addi x31, x31, 1           # x31 = 1
37  bne  x1, x2, 8
38  addi x31, x31, 100         #   <-- POISON
39  addi x31, x31, 1           # x31 = 2
40  blt  x3, x1, 8            # -8 < 10 (signed)
41  addi x31, x31, 100         #   <-- POISON
42  addi x31, x31, 1           # x31 = 3
43  bge  x1, x3, 8            # 10 >= -8 (signed)
44  addi x31, x31, 100         #   <-- POISON
45  addi x31, x31, 1           # x31 = 4
46  bltu x1, x3, 8            # 10 < 0xFFFFFFF8 (unsigned)
47  addi x31, x31, 100         #   <-- POISON
48  addi x31, x31, 1           # x31 = 5
49  bgeu x3, x1, 8            # 0xFFFFFFF8 >= 10 (unsigned)
50  addi x31, x31, 100         #   <-- POISON
51  addi x31, x31, 1           # x31 = 6
# --- jumps (2) ---
52  jal  x4, 8                # x4 = return addr (PC+4); jump -> skip poison
53  addi x31, x31, 100         #   <-- POISON
54  addi x31, x31, 1           # x31 = 7
55  auipc x4, 0x0              # x4 = PC = 55*4 = 220
56  jalr x4, x4, 12            # target = 220+12 = 232 (idx 58); x4 = return addr
57  addi x31, x31, 100         #   <-- POISON
58  addi x31, x31, 1           # x31 = 8
# --- halt ---
59  beq  x0, x0, 0             # infinite self-loop (stops the CPU)

# ============================================================
#  EXPECTED FINAL REGISTER VALUES (report.txt, 32-bit hex)
# ------------------------------------------------------------
#   x1  = 0000000a    x12 = ffffffff    x23 = fffffffc
#   x2  = 00000003    x13 = 0000000b    x24 = 12345000
#   x3  = fffffff8    x14 = 00000002    x25 = 0000005c
#   x5  = 0000000d    x15 = 0000000f    x26 = 0000000d
#   x6  = 00000007    x16 = 00000001    x27 = fffffff8
#   x7  = 00000050    x17 = 00000001    x28 = 000000f8
#   x8  = 00000001    x18 = 0000000c    x29 = 00000028
#   x9  = 00000000    x19 = 0000000f    x30 = 00000028
#   x10 = 00000009    x20 = 00000002    x31 = 00000008  <-- all branches+jumps OK
#   x11 = 00000001    x21 = 00000028
#   (x4 = return addr from jalr = 000000e4 = 228)
#
#  EXPECTED DMEM (byte view):
#   word[0]  (bytes 0-3)  = 0x0000000d   (sw x5)
#   half@8                = 0x0028        (sh x21 = 40)
#   byte@12               = 0xf8          (sb x3)
# ============================================================
