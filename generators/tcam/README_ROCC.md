# TCAM RoCC Integration

This document describes the integration of the TCAM (Ternary Content Addressable Memory) accelerator with the RoCC (Rocket Custom Coprocessor) interface in Chipyard.

## Overview

The TCAM RoCC accelerator provides a RISC-V custom instruction interface to TCAM functionality, allowing software to perform high-speed content-addressable memory operations using custom instructions rather than memory-mapped I/O.

## Features

- **Custom Instruction Interface**: Uses RISC-V custom instruction encoding space
- **Multiple TCAM Operations**: Write, search, status read, and configuration
- **Configurable Sizes**: Support for 32x28 and 64x28 TCAM configurations
- **Priority Encoding**: Hardware priority encoder for match resolution
- **Multi-opcode Support**: Can be configured to use different custom opcodes

## RoCC Commands

The TCAM RoCC accelerator supports the following commands via the `funct` field:

| Command | Value | Description | Usage |
|---------|-------|-------------|-------|
| TCAM_WRITE | 0 | Write data to TCAM | `tcam_write(data, address)` |
| TCAM_SEARCH | 1 | Search TCAM for pattern | `result = tcam_search(query)` |
| TCAM_STATUS | 2 | Read current status | `status = tcam_status()` |
| TCAM_CONFIG | 3 | Configure TCAM | `config = tcam_config()` |

## Usage

### C Code Example

```c
#include "tcam_rocc.h"

int main() {
    // Write data to TCAM
    tcam_write(0x12345678, 0);  // Write pattern to address 0
    tcam_write(0xABCDEF00, 1);  // Write pattern to address 1
    
    // Search for patterns
    uint32_t result1 = tcam_search(0x12345678);  // Should return 0
    uint32_t result2 = tcam_search(0xABCDEF00);  // Should return 1
    uint32_t result3 = tcam_search(0xDEADBEEF);  // Should return no-match value
    
    // Check status
    uint32_t status = tcam_status();
    
    return 0;
}
```

### Assembly Example

```assembly
# Write 0x12345678 to TCAM address 0
li t0, 0x12345678
li t1, 0
custom0 x0, t0, t1, 0  # TCAM_WRITE

# Search for pattern 0x12345678
li t0, 0x12345678
custom0 t2, t0, x0, 1  # TCAM_SEARCH, result in t2
```

## Configuration Options

### Basic TCAM RoCC Configuration

```scala
class MyTCAMConfig extends Config(
  new tcam.WithTCAMRoCC64x28 ++              // 64-entry, 28-bit key TCAM
  new freechips.rocketchip.subsystem.WithNBigCores(1) ++
  new chipyard.config.AbstractConfig)
```

### Dual TCAM Configuration

```scala
class DualTCAMConfig extends Config(
  new tcam.WithTCAMRoCC32x28(OpcodeSet.custom1) ++  // 32-entry TCAM on custom1
  new tcam.WithTCAMRoCC64x28(OpcodeSet.custom0) ++  // 64-entry TCAM on custom0
  new freechips.rocketchip.subsystem.WithNBigCores(1) ++
  new chipyard.config.AbstractConfig)
```

### TCAM with Other Accelerators

```scala
class TCAMGemminiConfig extends Config(
  new tcam.WithTCAMRoCC64x28(OpcodeSet.custom1) ++  // TCAM on custom1
  new gemmini.DefaultGemminiConfig ++               // Gemmini on custom0
  new freechips.rocketchip.subsystem.WithNBigCores(1) ++
  new chipyard.config.AbstractConfig)
```

## Available Configurations

The following pre-defined configurations are available:

- `TCAMRoCCConfig`: Single 64x28 TCAM on custom0
- `DualTCAMRoCCConfig`: Both 32x28 and 64x28 TCAMs
- `TCAMRoCCBoomConfig`: TCAM with BOOM core
- `TCAMGemminiRoCCConfig`: TCAM with Gemmini accelerator

## Building and Testing

### Build RTL

```bash
cd $CHIPYARD_ROOT
make CONFIG=TCAMRoCCConfig
```

### Run Tests

```bash
# Build the test
cd tests
make tcam_rocc_test.riscv

# Run in simulation
cd $CHIPYARD_ROOT/sims/verilator
make CONFIG=TCAMRoCCConfig run-binary BINARY=$CHIPYARD_ROOT/tests/tcam_rocc_test.riscv
```

## Technical Details

### Interface Timing

- **Write Operations**: 3-cycle operation (setup, hold, complete)
- **Search Operations**: 3-cycle operation (setup, search, result)
- **Status Operations**: 1-cycle operation
- **Busy Signal**: Asserted during multi-cycle operations

### Memory Layout

The TCAM accelerator uses the following address mapping for RoCC operations:
- Address bits [27:0] are used for TCAM addressing
- Data is 32-bit wide
- Priority encoder returns the lowest matching address

### Error Handling

- Invalid addresses are silently ignored
- Search misses return a value >= number of entries
- Configuration commands always return success (0x1)

## Performance Considerations

- RoCC interface provides lower latency than MMIO
- Searches complete in 3 cycles after instruction issue
- Multiple outstanding operations are not supported (busy signal)
- Priority encoding adds minimal delay to search operations

## Limitations

- Fixed 32-bit data width
- No support for masked/ternary operations in current implementation
- Single outstanding operation at a time
- No interrupt support for completion notification

## Future Enhancements

- Support for ternary (don't care) bits
- Configurable data widths
- Burst write/read operations
- Interrupt-driven completion
- Multiple outstanding operations