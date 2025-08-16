#ifndef __TCAM_ROCC_H__
#define __TCAM_ROCC_H__

#include <stdint.h>
#include <stdio.h>
#include "rocc.h"

// ==== TCAM RoCC Implementation ====

static uint32_t tcam_result = 0;
static uint32_t last_query_addr = 0;

// Low-level TCAM write with explicit control signals
static inline void tcam_write(uint32_t address, uint32_t wdata, uint8_t in_web) {
    uint32_t wmask = 0xF; // write all bits
    uint8_t in_csb = 0;   // always active

    uint64_t rs1 = ((uint64_t)(wmask & 0xF) << 28) | (address & 0x0FFFFFFF);
    uint64_t rs2 = (uint64_t)wdata;
    uint64_t result;

    // Manually dispatch based on funct value
    if (in_web == 0 && in_csb == 0) {
        ROCC_INSTRUCTION_DSS(0, result, rs1, rs2, 0); // funct = 0b00
    } else if (in_web == 0 && in_csb == 1) {
        ROCC_INSTRUCTION_DSS(0, result, rs1, rs2, 1); // funct = 0b01
    } else if (in_web == 1 && in_csb == 0) {
        ROCC_INSTRUCTION_DSS(0, result, rs1, rs2, 2); // funct = 0b10
    } else if (in_web == 1 && in_csb == 1) {
        ROCC_INSTRUCTION_DSS(0, result, rs1, rs2, 3); // funct = 0b11
    }
    tcam_result = (uint32_t)result;
}

// High-level write function (web=0 → write)
static inline void write_tcam(uint32_t tcam_addr, uint32_t wdata) {    
    tcam_write(tcam_addr, wdata, /*in_web=*/0);  // web=0 (write)
}

// Search function (web=1 → read/query)
static inline void search_tcam(uint32_t query) {
    tcam_write(query, 0, /*in_web=*/1);  // web=1 (read), address = query
    last_query_addr = query;
}

// Read TCAM match status (priority encoder output)
static inline uint32_t read_tcam_status() {
    uint64_t result;
    ROCC_INSTRUCTION_D(0, result, 3);
    tcam_result = (uint32_t)result;
    return tcam_result;
}

#define write_tcam_array(data_array) do { \
    int size = sizeof(data_array) / sizeof(data_array[0]); \
    for (int i = 0; i < size; i++) { \
        write_tcam(i, data_array[i]); \
    } \
} while(0)

#endif // __TCAM_ROCC_H__

