#include <stdio.h>
#include <stdint.h>
#include "tcam_rocc.h"

// ==== Test Application ====

int main() {
    printf("=== TCAM RoCC Test ===\n");

    // Populate TCAM with example values
    write_tcam(0x00000005,0x00000010);
    write_tcam(0x00000085,0x00000000);
    write_tcam(0x00000105,0x00000010);
    write_tcam(0x00000185,0x00000000);
    write_tcam(0x00000205,0x00000010);
    write_tcam(0x00000285,0x00000000);
    write_tcam(0x00000305,0x00000010);
    write_tcam(0x00000385,0x00000000);

    // Search for a value in TCAM
    uint32_t search_query = 0x00A14285;
    search_tcam(search_query);

    printf("TCAM match status: 0x%08X\n", read_tcam_status());

    return 0;
}

