#ifndef __TCAM_ROCC_H__
#define __TCAM_ROCC_H__

#include <stdint.h>

// TCAM RoCC Commands
#define TCAM_WRITE    0
#define TCAM_SEARCH   1
#define TCAM_STATUS   2
#define TCAM_CONFIG   3

// RoCC instruction macros for TCAM
#define ROCC_INSTRUCTION_DSS(x, rd, rs1, rs2, funct) \
  asm volatile ("custom0 %0, %1, %2, %3" : "=r"(rd) : "r"(rs1), "r"(rs2), "i"(funct))

#define ROCC_INSTRUCTION_DS(x, rd, rs1, funct) \
  asm volatile ("custom0 %0, %1, x0, %2" : "=r"(rd) : "r"(rs1), "i"(funct))

#define ROCC_INSTRUCTION_D(x, rd, funct) \
  asm volatile ("custom0 %0, x0, x0, %1" : "=r"(rd) : "i"(funct))

#define ROCC_INSTRUCTION_SS(x, rs1, rs2, funct) \
  asm volatile ("custom0 x0, %0, %1, %2" : : "r"(rs1), "r"(rs2), "i"(funct))

#define ROCC_INSTRUCTION_S(x, rs1, funct) \
  asm volatile ("custom0 x0, %0, x0, %1" : : "r"(rs1), "i"(funct))

#define ROCC_INSTRUCTION(x, funct) \
  asm volatile ("custom0 x0, x0, x0, %0" : : "i"(funct))

// TCAM specific instruction wrappers
static inline void tcam_write(uint32_t data, uint32_t address) {
    ROCC_INSTRUCTION_SS(0, data, address, TCAM_WRITE);
}

static inline uint32_t tcam_search(uint32_t query) {
    uint32_t result;
    ROCC_INSTRUCTION_DS(0, result, query, TCAM_SEARCH);
    return result;
}

static inline uint32_t tcam_status(void) {
    uint32_t result;
    ROCC_INSTRUCTION_D(0, result, TCAM_STATUS);
    return result;
}

static inline uint32_t tcam_config(void) {
    uint32_t result;
    ROCC_INSTRUCTION_D(0, result, TCAM_CONFIG);
    return result;
}

#endif // __TCAM_ROCC_H__