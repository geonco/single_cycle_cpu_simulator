/* **************************************
 * Module: top design of rv32i single-cycle processor
 *
 * Author:
 *
 * **************************************
 */

// headers
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


// defines
#define REG_WIDTH 32
#define IMEM_DEPTH 1024
#define DMEM_DEPTH 1024

// configs
#define CLK_NUM 45

// structures
struct imem_input_t {
	uint32_t addr;
};
struct imem_output_t {
	uint32_t dout;
};

struct regfile_input_t {
	uint32_t rs1;
	uint32_t rs2;
	uint32_t rd;
	uint32_t rd_din;
	uint32_t reg_write;
};
struct regfile_output_t {
	uint32_t rs1_dout;
	uint32_t rs2_dout;
};

struct alu_input_t {
	uint32_t in1;
	uint32_t in2;
	uint32_t alu_control;		
};
struct alu_output_t {
	uint32_t result;
	uint32_t zero;
	uint32_t sign;		
};

// unaligned dmem
struct dmem_input_t {
	uint32_t addr;
	uint32_t rd_en;		// mem_read
	uint32_t wr_en;		// mem_write
	uint32_t sz;
	uint32_t din;
};
struct dmem_output_t {
	uint32_t dout;
};

struct control_input_t {
	uint32_t inst;
};
struct control_output_t {
	uint32_t branch;
	uint32_t mem_read;
	uint32_t mem_write;
	uint32_t mem_to_reg;
	uint32_t reg_write;
	uint32_t alu_src;
	uint32_t alu_op;
	uint32_t alu_control;
	uint32_t lui;
	uint32_t auipc;
	uint32_t jal;
	uint32_t jalr;
	uint32_t sz;
};

struct imem_output_t imem(struct imem_input_t in, uint32_t *imem_data);
struct regfile_output_t regfile(struct regfile_input_t in, uint32_t *reg_data);
struct alu_output_t alu(struct alu_input_t in);
struct dmem_output_t dmem(struct dmem_input_t in, uint32_t *dmem_data);
struct control_output_t control(struct control_input_t in);