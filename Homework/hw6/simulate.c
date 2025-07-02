#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "cache.h"
#include "simulate.h"

#define REGISTER_NAME_TO_NUMBER(name1, name2, number) \
  if (strcmp(register_name, name1) == 0 || strcmp(register_name, name2) == 0) return number

#define PARSE_LW(inst_name) \
  else if (strcmp(inst, #inst_name) == 0) { \
    instruction->type = INSTRUCTION_ ## inst_name; \
    instruction->rd = register_name_to_label(strtok(NULL, ", ")); \
    instruction->imm = strtol(strtok(NULL, " ("), NULL, 10); \
    instruction->rs1 = register_name_to_label(strtok(NULL, ")")); \
    fprintf(stderr, "rs1: %u, rd: %u, imm: %ld\n", instruction->rs1, instruction->rd, instruction->imm); \
  }

#define PARSE_SW(inst_name) \
  else if (strcmp(inst, #inst_name) == 0) { \
    instruction->type = INSTRUCTION_ ## inst_name; \
    instruction->rs2 = register_name_to_label(strtok(NULL, ", ")); \
    instruction->imm = strtol(strtok(NULL, " ("), NULL, 10); \
    instruction->rs1 = register_name_to_label(strtok(NULL, ")")); \
    fprintf(stderr, "rs1: %u, rs2: %u, imm: %ld\n", instruction->rs1, instruction->rs2, instruction->imm); \
  }

/* Finish the remaining macro. */
#define PARSE_RTYPE(inst_name) \
  else if (strcmp(inst, #inst_name) == 0) { \
    instruction->type = INSTRUCTION_ ## inst_name; \
    instruction->rd = register_name_to_label(strtok(NULL, ", ")); \
    instruction->rs1 = register_name_to_label(strtok(NULL, ", ")); \
    instruction->rs2 = register_name_to_label(strtok(NULL, " \n")); \
    fprintf(stderr, "rs1: %u, rs2: %u, rd: %u\n", instruction->rs1, instruction->rs2, instruction->rd); \
  }

#define PARSE_ITYPE(inst_name) \
  else if (strcmp(inst, #inst_name) == 0) { \
    instruction->type = INSTRUCTION_ ## inst_name; \
    instruction->rd = register_name_to_label(strtok(NULL, ", ")); \
    instruction->rs1 = register_name_to_label(strtok(NULL, ", ")); \
    instruction->imm = strtol(strtok(NULL, " \n"), NULL, 10); \
    fprintf(stderr, "rs1: %u, rd: %u, imm: %ld\n", instruction->rs1, instruction->rd, instruction->imm); \
  }

#define PARSE_SBTYPE(inst_name) \
  else if (strcmp(inst, #inst_name) == 0) { \
    instruction->type = INSTRUCTION_ ## inst_name; \
    instruction->rs1 = register_name_to_label(strtok(NULL, ", ")); \
    instruction->rs2 = register_name_to_label(strtok(NULL, ", ")); \
    instruction->imm = strtol(strtok(NULL, " \n"), NULL, 10); \
    fprintf(stderr, "rs1: %u, rs2: %u, imm: %ld\n", instruction->rs1, instruction->rs2, instruction->imm); \
  }

/* 32 integer registers. */
static uint32_t registers[32];

/* The program counter. */
static uint32_t program_counter;

static unsigned register_name_to_label(const char *register_name) {
  // printf("Register name: %s\n", register_name);

  REGISTER_NAME_TO_NUMBER("x0", "zero", 0);
  REGISTER_NAME_TO_NUMBER("x1", "ra", 1);
  REGISTER_NAME_TO_NUMBER("x2", "sp", 2);
  REGISTER_NAME_TO_NUMBER("x3", "gp", 3);
  REGISTER_NAME_TO_NUMBER("x4", "tp", 4);
  REGISTER_NAME_TO_NUMBER("x5", "t0", 5);
  REGISTER_NAME_TO_NUMBER("x6", "t1", 6);
  REGISTER_NAME_TO_NUMBER("x7", "t2", 7);
  REGISTER_NAME_TO_NUMBER("x8", "s0", 8);
  REGISTER_NAME_TO_NUMBER("x9", "s1", 9);
  REGISTER_NAME_TO_NUMBER("x10", "a0", 10);
  REGISTER_NAME_TO_NUMBER("x11", "a1", 11);
  REGISTER_NAME_TO_NUMBER("x12", "a2", 12);
  REGISTER_NAME_TO_NUMBER("x13", "a3", 13);
  REGISTER_NAME_TO_NUMBER("x14", "a4", 14);
  REGISTER_NAME_TO_NUMBER("x15", "a5", 15);
  REGISTER_NAME_TO_NUMBER("x16", "a6", 16);
  REGISTER_NAME_TO_NUMBER("x17", "a7", 17);
  REGISTER_NAME_TO_NUMBER("x18", "s2", 18);
  REGISTER_NAME_TO_NUMBER("x19", "s3", 19);
  REGISTER_NAME_TO_NUMBER("x20", "s4", 20);
  REGISTER_NAME_TO_NUMBER("x21", "s5", 21);
  REGISTER_NAME_TO_NUMBER("x22", "s6", 22);
  REGISTER_NAME_TO_NUMBER("x23", "s7", 23);
  REGISTER_NAME_TO_NUMBER("x24", "s8", 24);
  REGISTER_NAME_TO_NUMBER("x25", "s9", 25);
  REGISTER_NAME_TO_NUMBER("x26", "s10", 26);
  REGISTER_NAME_TO_NUMBER("x27", "s11", 27);
  REGISTER_NAME_TO_NUMBER("x28", "t3", 28);
  REGISTER_NAME_TO_NUMBER("x29", "t4", 29);
  REGISTER_NAME_TO_NUMBER("x30", "t5", 30);
  REGISTER_NAME_TO_NUMBER("x31", "t6", 31);
  if (strcmp(register_name, "fp") == 0) return 8;
  //printf(23421234);
  return 32;
}

static void parse_inst(char *buf, struct Instruction *instruction) {
  char *inst = strtok(buf, " ");
  fprintf(stderr, "Instruction: %s, ", inst);

  if (strcmp(inst, "li") == 0) {
    instruction->type = INSTRUCTION_li;
    instruction->rd = register_name_to_label(strtok(NULL, ", "));
    instruction->imm = strtol(strtok(NULL, " \n"), NULL, 0);
    fprintf(stderr, "rd: %u, imm: %ld\n", instruction->rd, instruction->imm);
  }
  PARSE_LW(lw)
  PARSE_SW(sw)
  PARSE_RTYPE(add)
  PARSE_RTYPE(and)
  PARSE_RTYPE(div)
  PARSE_RTYPE(mul)
  PARSE_RTYPE(rem)
  PARSE_RTYPE(or)
  PARSE_RTYPE(sll)
  PARSE_RTYPE(srl)
  PARSE_RTYPE(sub)
  PARSE_RTYPE(xor)
  PARSE_ITYPE(addi)
  PARSE_ITYPE(andi)
  PARSE_ITYPE(ori)
  PARSE_ITYPE(slli)
  PARSE_ITYPE(srli)
  PARSE_ITYPE(xori)
  PARSE_SBTYPE(beq)
  PARSE_SBTYPE(bne)
  
  /* Use the PARSE_* macro you written above. */
  /* Your code here. */

  else {
    fprintf(stderr, "Invalid instruction!\n");
  }
}

struct Instruction *parse_asm(FILE *file, size_t num_lines) {
  struct Instruction *instructions = malloc(sizeof(struct Instruction) * num_lines);
  size_t i;
  char buf[100];
  for (i = 0; i < num_lines; ++i) {
    fgets(buf, sizeof(buf), file);
    parse_inst(buf, instructions + i);
  }
  return instructions;
}

/***************************
 *                         *
 *  YOUR CODE START HERE.  *
 *                         *
 ***************************/
/* Initialize the registers and program counter. */
void init_registers() {
  memset(registers, 0, sizeof(registers));
  program_counter = 0;
}

void start_simulation(struct Instruction *instructions, size_t num) {
  /* Your code here. */
  init_cache_system();
  init_registers();
  program_counter = 0;
  //printf("%zu\n", num);
  while(program_counter < (num<<2)) {
    //printf("PC: %u\n", program_counter);
    // for(int i = 0; i < 32; i++) {
    //   printf("registers[%d]: %u\n", i, registers[i]);
    // }
    add_inst_access(program_counter);
    struct Instruction *inst = instructions + program_counter / 4;
    if (inst->type == INSTRUCTION_li) {
    registers[inst->rd] = inst->imm;
    program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_lw) {
        add_data_access(registers[inst->rs1] + inst->imm);
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_sw) {
        add_data_access(registers[inst->rs1] + inst->imm);
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_add) {
        registers[inst->rd] = registers[inst->rs1] + registers[inst->rs2];
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_and) {
        registers[inst->rd] = registers[inst->rs1] & registers[inst->rs2];
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_div) {
        registers[inst->rd] = registers[inst->rs1] / registers[inst->rs2];
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_mul) {
        registers[inst->rd] = registers[inst->rs1] * registers[inst->rs2];
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_rem) {
        registers[inst->rd] = registers[inst->rs1] % registers[inst->rs2];
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_or) {
        registers[inst->rd] = registers[inst->rs1] | registers[inst->rs2];
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_sll) {
        registers[inst->rd] = registers[inst->rs1] << (registers[inst->rs2] & 0x1F);
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_srl) {
        registers[inst->rd] = (uint32_t)registers[inst->rs1] >> (registers[inst->rs2] & 0x1F);
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_sub) {
        registers[inst->rd] = registers[inst->rs1] - registers[inst->rs2];
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_xor) {
        registers[inst->rd] = registers[inst->rs1] ^ registers[inst->rs2];
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_addi) {
        registers[inst->rd] = registers[inst->rs1] + inst->imm;
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_andi) {
        registers[inst->rd] = registers[inst->rs1] & inst->imm;
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_ori) {
        registers[inst->rd] = registers[inst->rs1] | inst->imm;
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_slli) {
        registers[inst->rd] = registers[inst->rs1] << (inst->imm & 0x1F);
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_srli) {
        registers[inst->rd] = (uint32_t)registers[inst->rs1] >> (inst->imm & 0x1F);
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_xori) {
        registers[inst->rd] = registers[inst->rs1] ^ inst->imm;
        program_counter += 4;
    }
    else if (inst->type == INSTRUCTION_beq) {
        if (registers[inst->rs1] == registers[inst->rs2]) {
            program_counter += inst->imm;
        } else {
            program_counter += 4;
        }
    }
    else if (inst->type == INSTRUCTION_bne) {
        if (registers[inst->rs1] != registers[inst->rs2]) {
            program_counter += inst->imm;
        } else {
            program_counter += 4;
        }
    }
  }

}
