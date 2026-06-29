/* **************************************
 * Module: top design of rv32i single-cycle processor
 *
 * Author:
 *
 * **************************************
 */

#include "rv32i.h"

int main (int argc, char *argv[]) {

	// get input arguments
	FILE *f_imem, *f_dmem;
	if (argc < 3) {
		printf("usage: %s imem_data_file dmem_data_file\n", argv[0]);
		exit(1);
	}

	if ( (f_imem = fopen(argv[1], "r")) == NULL ) {
		printf("Cannot find %s\n", argv[1]);
		exit(1);
	}
	if ( (f_dmem = fopen(argv[2], "r")) == NULL ) {
		printf("Cannot find %s\n", argv[2]);
		exit(1);
	}

	// memory data (global)
	uint32_t *reg_data;
	uint32_t *imem_data;
	uint32_t *dmem_data;

	reg_data = (uint32_t*)malloc(32*sizeof(uint32_t));
	imem_data = (uint32_t*)malloc(IMEM_DEPTH*sizeof(uint32_t));
	dmem_data = (uint32_t*)malloc(DMEM_DEPTH*sizeof(uint32_t));

	// initialize memory data
	int i, j, k;
	for (i = 0; i < 32; i++) reg_data[i] = 0;
	for (i = 0; i < IMEM_DEPTH; i++) imem_data[i] = 0;
	for (i = 0; i < DMEM_DEPTH; i++) dmem_data[i] = 0;

	uint32_t d, buf;
	i = 0;
	printf("\n*** Reading %s ***\n", argv[1]);
	while (fscanf(f_imem, "%1d", &buf) != EOF) {
		d = buf << 31;
		for (k = 30; k >= 0; k--) {
			if (fscanf(f_imem, "%1d", &buf) != EOF) {
				d |= buf << k;
			} else {
				printf("Incorrect format!!\n");
				exit(1);
			}
		}
		imem_data[i] = d;
		printf("imem[%03d]: %08X\n", i, imem_data[i]);
		i++;
	}

	i = 0;
	printf("\n*** Reading %s ***\n", argv[2]);
	while (fscanf(f_dmem, "%8x", &buf) != EOF) {
		dmem_data[i] = buf;
		printf("dmem[%03d]: %08X\n", i, dmem_data[i]);
		i++;
	}

	fclose(f_imem);
	fclose(f_dmem);

	// processor model
	uint32_t pc_curr, pc_next;	// program counter
	struct imem_input_t imem_in;
	struct imem_output_t imem_out;
	struct regfile_input_t regfile_in;
	struct regfile_output_t regfile_out;
	struct alu_input_t alu_in;
	struct alu_output_t alu_out;
	struct dmem_input_t dmem_in;
	struct dmem_output_t dmem_out;
	struct control_input_t control_in;
	struct control_output_t control_out;
	struct imm_input_t imm_in;
	struct imm_output_t imm_out;

	pc_curr = 0;

	uint32_t cc = 2;	// clock count
	while (cc < CLK_NUM) {
		// instruction fetch
		imem_in.addr = pc_curr;
		imem_out = imem(imem_in, imem_data);
		uint32_t inst = imem_out.dout;
		
		// instruction decode
		control_in.inst = inst;
		control_out = control(control_in);

		imm_in.inst = inst;
		imm_out = immgen(imm_in);

		uint32_t funct3 = (inst >> 12) & 0x7;
		regfile_in.rs1 = (inst >> 15) & 0x1f;
		regfile_in.rs2 = (inst >> 20) & 0x1f;
		regfile_in.rd = (inst >> 7) & 0x1f;
		regfile_in.reg_write = 0;
		regfile_out = regfile(regfile_in, reg_data);

		// execution
		alu_in.in1 = regfile_out.rs1_dout;
		alu_in.in2 = (control_out.alu_src == 1)? imm_out.imm32 : regfile_out.rs2_dout;
		alu_in.alu_control = control_out.alu_control;
		alu_out = alu(alu_in);

		// memory
		dmem_in.addr = alu_out.result;
		dmem_in.din = regfile_out.rs2_dout;
		dmem_in.rd_en = control_out.mem_read;
		dmem_in.sz = control_out.sz;
		dmem_in.wr_en = control_out.mem_write;
		dmem_out = dmem(dmem_in, dmem_data);

		uint32_t dmem_dout_ext = dmem_out.dout;
		switch (funct3)
		{
			case 0:	dmem_dout_ext |= (dmem_out.dout >> 7 & 0x1) ? 0xffffff00 : 0x00000000;	break;
			case 1: dmem_dout_ext |= (dmem_out.dout >> 15 & 0x1) ? 0xffff0000 : 0x00000000;	break;
			case 2: dmem_dout_ext = dmem_dout_ext;	break;
			case 4:	dmem_dout_ext &= 0x000000ff;	break;
			case 5:	dmem_dout_ext &= 0x0000ffff;	break;
			default:	dmem_dout_ext = dmem_dout_ext;	break;
		}
		dmem_out.dout = dmem_dout_ext;

		// write-back
		uint32_t rd_din;
		if (control_out.lui)							rd_din = imm_out.imm32_u;
		else if (control_out.auipc)						rd_din = pc_curr + imm_out.imm32_u;
		else if (control_out.jal || control_out.jalr)	rd_din = pc_curr + 4;
		else if (control_out.mem_to_reg)				rd_din = dmem_dout_ext;
		else											rd_din = alu_out.result;

		regfile_in.rd_din =	rd_din;
		regfile_in.reg_write = control_out.reg_write;
		regfile_out = regfile(regfile_in, reg_data);
		
		// pc update
		uint32_t rs1_lt_rs2_u = (regfile_out.rs1_dout < regfile_out.rs2_dout);
		uint32_t taken = ((control_out.branch >> 0) & 1 & alu_out.zero) || 
						 ((control_out.branch >> 1) & 1 & ~alu_out.zero) ||
						 ((control_out.branch >> 2) & 1 & alu_out.sign) ||
						 ((control_out.branch >> 3) & 1 & ~alu_out.sign) ||
						 ((control_out.branch >> 4) & 1 & rs1_lt_rs2_u) ||
						 ((control_out.branch >> 5) & 1 & ~rs1_lt_rs2_u);

		if (control_out.jal)			pc_next = pc_curr + imm_out.imm32_jal;
		else if (control_out.jalr)		pc_next = regfile_out.rs1_dout + imm_out.imm32;
		else if (taken)					pc_next = pc_curr + imm_out.imm32_branch;
		else							pc_next = pc_curr + 4;

		pc_curr = pc_next;

		cc++;
	}

	// dump register file & data memory to report.txt
	FILE *f_report = fopen("report.txt", "w");
	for (i = 0; i < 32; i++) fprintf(f_report, "RF[%02d]: %08x\n", i, reg_data[i]);
	for (i = 0; i < 8;  i++) fprintf(f_report, "DMEM[%02d]: %08x\n", i, dmem_data[i]);
	fclose(f_report);

	free(reg_data);
	free(imem_data);
	free(dmem_data);

	return 1;
}

struct imem_output_t imem(struct imem_input_t in, uint32_t *imem_data) {
	struct imem_output_t out;
	out.dout = imem_data[in.addr >> 2];
	return out;
}

struct regfile_output_t regfile(struct regfile_input_t in, uint32_t *reg_data) {
	struct regfile_output_t out;
	out.rs1_dout = (in.rs1 == 0)? 0:reg_data[in.rs1];
	out.rs2_dout = (in.rs2 == 0)? 0:reg_data[in.rs2];
	if (in.reg_write == 1 && in.rd !=0) reg_data[in.rd] = in.rd_din;
	return out;
}

struct alu_output_t alu(struct alu_input_t in) {
	struct alu_output_t out;
	switch (in.alu_control)
	{
		case 0x0:	out.result = in.in1 & in.in2;	break;
		case 0x1:	out.result = in.in1 | in.in2;	break;
		case 0x2:	out.result = in.in1 + in.in2;	break;
		case 0x3:	out.result = in.in1 ^ in.in2;	break;
		case 0x6:	out.result = in.in1 - in.in2;	break;
		case 0x4:	out.result = in.in1 << (in.in2 & 0x1f);	break;
		case 0x5:	out.result = in.in1 >> (in.in2 & 0x1f);	break;
		case 0x8:	out.result = (int32_t)in.in1 >> (in.in2 & 0x1f);	break;
		case 0x7:	out.result = ((int32_t)in.in1 < (int32_t)in.in2)? 1:0;	break;
		case 0x9:	out.result = (in.in1 < in.in2)? 1:0;	break;
		default:	out.result = in.in1 + in.in2;	break;
	}

	out.zero = (out.result == 0)? 1:0;
	out.sign = (out.result >> 31) & 1;
	return out;
}

struct dmem_output_t dmem(struct dmem_input_t in, uint32_t *dmem_data) {
	struct dmem_output_t out;
	out.dout = 0;

	// 접근 바이트 수: sz 0=byte(1), 1=half(2), 2=word(4)
	uint32_t nbytes = (in.sz == 0)? 1 : (in.sz == 1)? 2 : 4;
	uint32_t b;

	// 바이트 단위로 처리 -> 워드 경계 넘는 unaligned 접근도 동작 (little-endian)
	if (in.rd_en == 1) {
		for (b = 0; b < nbytes; b++) {
			uint32_t a = in.addr + b;			// 바이트 주소
			uint32_t widx = a >> 2;				// 몇번째 워드
			uint32_t boff = (a & 3) * 8;		// 워드 안 비트 오프셋
			uint32_t byte = (dmem_data[widx] >> boff) & 0xff;
			out.dout |= byte << (b * 8);		// 낮은 주소가 LSB
		}
	}

	if (in.wr_en == 1) {
		for (b = 0; b < nbytes; b++) {
			uint32_t a = in.addr + b;
			uint32_t widx = a >> 2;
			uint32_t boff = (a & 3) * 8;
			uint32_t byte = (in.din >> (b * 8)) & 0xff;
			dmem_data[widx] = (dmem_data[widx] & ~(0xffu << boff)) | (byte << boff);
		}
	}

	return out;
}

struct control_output_t control(struct control_input_t in) {
	struct control_output_t out;

	uint32_t opcode = in.inst & 0x7f;
	uint32_t funct3 = (in.inst >> 12) & 0x7;
	uint32_t funct7 = (in.inst >> 25) & 0x7f;

	out.branch = 0;
	out.branch |= ((opcode == 0x63) && (funct3 == 0x0)) << 0;
	out.branch |= ((opcode == 0x63) && (funct3 == 0x1)) << 1;
	out.branch |= ((opcode == 0x63) && (funct3 == 0x4)) << 2;
	out.branch |= ((opcode == 0x63) && (funct3 == 0x5)) << 3;
	out.branch |= ((opcode == 0x63) && (funct3 == 0x6)) << 4;
	out.branch |= ((opcode == 0x63) && (funct3 == 0x7)) << 5;

	out.lui = (opcode == 0x37);
	out.auipc = (opcode == 0x17);
	
	out.jal = (opcode == 0x6f);
	out.jalr = (opcode == 0x67);

	out.mem_read = (opcode == 0x03);
	out.mem_write = (opcode == 0x23);
	out.mem_to_reg = (opcode == 0x03);
	out.reg_write = (opcode == 0x03) || (opcode == 0x33) || (opcode == 0x13) || out.lui || out.auipc || out.jal || out.jalr;
	out.alu_src = (opcode == 0x03) || (opcode == 0x23) || (opcode == 0x13); 

	out.alu_op = 0;
	out.alu_op |= (opcode == 0x63);
	out.alu_op |= ((opcode == 0x33) || (opcode == 0x13)) << 1;

	out.sz = funct3 & 0x3;
	
	switch (out.alu_op)
	{
		case 0: out.alu_control = 0x2;	break;
		case 1: out.alu_control = 0x6;	break;
		case 2: switch (funct3)
		{
			case 0:	out.alu_control = (funct7 == 0x20 && opcode == 0x33)? 0x6 : 0x2;	break;
			case 1:	out.alu_control = 0x4;	break;
			case 2:	out.alu_control = 0x7;	break;
			case 3:	out.alu_control = 0x9;	break;
			case 4:	out.alu_control = 0x3;	break;
			case 5:	out.alu_control = (funct7 == 0x20)? 0x8 : 0x5;	break;
			case 6:	out.alu_control = 0x1;	break;
			case 7:	out.alu_control = 0x0;	break;
			default:	out.alu_control = 0x2;	break;
		}	break;
		default: out.alu_control = 0x2;
	}

	return out;
}

struct imm_output_t immgen(struct imm_input_t in) {
	struct imm_output_t out;
	uint32_t opcode = in.inst & 0x7f;
	uint32_t imm12;

	switch (opcode)
	{
		case 0x23:	imm12 = ((in.inst >> 25 & 0x7f) << 5 | (in.inst >> 7 & 0x1f));	break;
		case 0x63:	imm12 = ((in.inst >> 31 & 0x1) << 11 | (in.inst >> 7 & 0x1) << 10 | (in.inst >> 25 & 0x3f) << 4 | (in.inst >> 8 & 0xf));	break; 
		default:	imm12 = (in.inst >> 20 & 0xfff);	break;
	}

	out.imm32 = (imm12 & 0x800)? (0xfffff000 | imm12) : imm12;
	out.imm32_branch = out.imm32 << 1;

	out.imm32_u = ((in.inst >> 12) & 0xfffff) << 12 & 0xfffff000;
	
	uint32_t jal = (in.inst >> 31 & 0x1) << 20 | (in.inst >> 12 & 0xff) << 12 | (in.inst >> 20 & 0x1) << 11 | (in.inst >> 21 & 0x3ff) << 1 | 0;
	out.imm32_jal = (jal & 0x100000)? (0xffe00000 | jal) : jal;

	return out;
}