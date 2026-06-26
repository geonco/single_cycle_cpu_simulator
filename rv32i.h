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