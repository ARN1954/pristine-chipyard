package tcam

import org.chipsalliance.cde.config.{Config, Parameters}
import freechips.rocketchip.subsystem.BaseSubsystem
import freechips.rocketchip.diplomacy.LazyModule

// Base configuration for TCAM
class WithTCAM(params: TCAMParams = TCAMParams()) extends Config((site, here, up) => {
  case TCAMKey => Some(params)
})

// 64x28 TCAM configuration
class WithTCAM64x28 extends Config(
  new WithTCAM(TCAMParams(
    width = 32,
    keyWidth = 28,  // queryStrLen
    entries = 64,   // potMatchAddr
    sramDepth = 512,
    sramWidth = 32,
    priorityEncoder = true,
    address = 0x4000,
    useAXI4 = false,
    tableConfig = Some(TCAMTableConfig(
      queryStrLen = 28,
      subStrLen = 7,
      totalSubStr = 4,
      potMatchAddr = 64
    ))
  ))
)

// 64x28 TCAM configuration with AXI4
class WithTCAM64x28AXI4 extends Config(
  new WithTCAM(TCAMParams(
    width = 32,
    keyWidth = 28,  // queryStrLen
    entries = 64,   // potMatchAddr
    sramDepth = 512,
    sramWidth = 32,
    priorityEncoder = true,
    address = 0x4000,
    useAXI4 = true,
    tableConfig = Some(TCAMTableConfig(
      queryStrLen = 28,
      subStrLen = 7,
      totalSubStr = 4,
      potMatchAddr = 64
    ))
  ))
)


