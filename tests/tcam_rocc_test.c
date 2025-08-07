#include <stdio.h>
#include <stdint.h>
#include "tcam_rocc.h"

// Test data patterns
uint32_t test_data[] = {
    0x12345678, 0xABCDEF00, 0xDEADBEEF, 0xCAFEBABE,
    0x11111111, 0x22222222, 0x33333333, 0x44444444,
    0xFFFFFFFF, 0x00000000, 0x55AA55AA, 0xAA55AA55,
    0x87654321, 0x13579BDF, 0x2468ACE0, 0xFEDCBA98
};

uint32_t search_queries[] = {
    0x12345678, 0xABCDEF00, 0xDEADBEEF, 0xCAFEBABE,
    0x99999999, 0x77777777, 0x00000000, 0xFFFFFFFF
};

int main() {
    printf("=== TCAM RoCC Test ===\n");
    
    // Test configuration
    printf("Testing TCAM configuration...\n");
    uint32_t config_result = tcam_config();
    printf("Config result: 0x%08X\n", config_result);
    
    // Test writing data to TCAM
    printf("\nWriting test data to TCAM...\n");
    int num_entries = sizeof(test_data) / sizeof(test_data[0]);
    
    for (int i = 0; i < num_entries; i++) {
        tcam_write(test_data[i], i);
        printf("Wrote 0x%08X to address %d\n", test_data[i], i);
    }
    
    printf("Write operations completed.\n");
    
    // Test searching TCAM
    printf("\nSearching TCAM...\n");
    int num_queries = sizeof(search_queries) / sizeof(search_queries[0]);
    
    for (int i = 0; i < num_queries; i++) {
        uint32_t query = search_queries[i];
        uint32_t result = tcam_search(query);
        printf("Query 0x%08X -> Match address: %d\n", query, result);
        
        // Verify the result by checking if it's a valid match
        if (result < num_entries && test_data[result] == query) {
            printf("  ✓ Valid match found at address %d\n", result);
        } else if (result >= num_entries) {
            printf("  ✗ No match found (result: %d)\n", result);
        } else {
            printf("  ⚠ Unexpected result (address %d contains 0x%08X)\n", 
                   result, test_data[result]);
        }
    }
    
    // Test status reading
    printf("\nReading TCAM status...\n");
    uint32_t status = tcam_status();
    printf("Current status: 0x%08X\n", status);
    
    // Performance test
    printf("\nPerformance test - 1000 searches...\n");
    uint32_t perf_query = 0x12345678;
    
    for (int i = 0; i < 1000; i++) {
        uint32_t result = tcam_search(perf_query);
        if (i == 0) {
            printf("First search result: %d\n", result);
        }
    }
    printf("Performance test completed.\n");
    
    // Edge case tests
    printf("\nTesting edge cases...\n");
    
    // Test with maximum address
    tcam_write(0xEDCECA5E, 63);  // Assuming 64-entry TCAM
    uint32_t edge_result = tcam_search(0xEDCECA5E);
    printf("Edge case (max addr): Query 0xEDCECA5E -> Result: %d\n", edge_result);
    
    // Test with non-existent pattern
    uint32_t noexist_result = tcam_search(0xDEADC0DE);
    printf("Non-existent pattern: Query 0xDEADC0DE -> Result: %d\n", noexist_result);
    
    // Test overwriting existing entry
    printf("Overwriting entry 0 with new data...\n");
    tcam_write(0x12EEDA7A, 0);
    uint32_t overwrite_result = tcam_search(0x12EEDA7A);
    printf("Overwrite test: Query 0x%08X -> Result: %d\n", 0x12EEDA7A, overwrite_result);
    
    // Verify old data is gone
    uint32_t old_result = tcam_search(test_data[0]);
    printf("Old data search: Query 0x%08X -> Result: %d\n", test_data[0], old_result);
    
    printf("\n=== TCAM RoCC Test Complete ===\n");
    
    return 0;
}