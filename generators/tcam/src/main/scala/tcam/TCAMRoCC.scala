package tcam

import chisel3._
import chisel3.util._
import freechips.rocketchip.tile._
import freechips.rocketchip.config._
import freechips.rocketchip.diplomacy._
import freechips.rocketchip.rocket.{HellaCacheReq, HellaCacheResp}
import freechips.rocketchip.util.DecoupledHelper

case object TCAMRoCCKey extends Field[Option[TCAMParams]](None)

// TCAM RoCC Commands
object TCAMCommands {
  val TCAM_WRITE    = 0.U(7.W)  // Write data to TCAM at address
  val TCAM_SEARCH   = 1.U(7.W)  // Search TCAM with query
  val TCAM_STATUS   = 2.U(7.W)  // Read status/result
  val TCAM_CONFIG   = 3.U(7.W)  // Configure TCAM parameters
}

class TCAMRoCC(opcodes: OpcodeSet, params: TCAMParams)(implicit p: Parameters) 
    extends LazyRoCC(opcodes) {
  override lazy val module = new TCAMRoCCModuleImp(this)
  
  class TCAMRoCCModuleImp(outer: TCAMRoCC) extends LazyRoCCModuleImp(outer) {
    
    // Instantiate the TCAM blackbox
    val tcam = Module(new TCAMBlackBox(params))
    
    // Get table configuration for output width
    val tableConfig = params.tableConfig.getOrElse(TCAMTableConfig(
      queryStrLen = params.keyWidth,
      subStrLen = params.keyWidth / 4,
      totalSubStr = 4,
      potMatchAddr = params.entries
    ))
    val outputWidth = log2Ceil(tableConfig.potMatchAddr)
    
    // Internal registers
    val busy = RegInit(false.B)
    val result = RegInit(0.U(outputWidth.W))
    val cmd_reg = Reg(new RoCCCommand)
    
    // Command interface
    val cmd = Queue(io.cmd)
    val funct = cmd.bits.inst.funct
    val rs1_data = cmd.bits.rs1
    val rs2_data = cmd.bits.rs2
    val rd = cmd.bits.inst.rd
    val xd = cmd.bits.inst.xd
    
    // Response interface
    io.resp.valid := false.B
    io.resp.bits.rd := cmd_reg.inst.rd
    io.resp.bits.data := result
    
    // Busy signal
    io.busy := busy
    
    // Default TCAM connections
    tcam.io.in_clk := clock
    tcam.io.in_csb := true.B  // Default to chip select disabled
    tcam.io.in_web := true.B  // Default to write disabled
    tcam.io.in_wmask := 0.U
    tcam.io.in_addr := 0.U
    tcam.io.in_wdata := 0.U
    
    // State machine for TCAM operations
    val s_idle :: s_write :: s_search :: s_status :: s_resp :: Nil = Enum(5)
    val state = RegInit(s_idle)
    
    // Operation counter for multi-cycle operations
    val op_counter = RegInit(0.U(8.W))
    
    cmd.ready := (state === s_idle)
    
    switch(state) {
      is(s_idle) {
        when(cmd.valid) {
          busy := true.B
          cmd_reg := cmd.bits
          
          switch(funct) {
            is(TCAMCommands.TCAM_WRITE) {
              state := s_write
              op_counter := 0.U
            }
            is(TCAMCommands.TCAM_SEARCH) {
              state := s_search  
              op_counter := 0.U
            }
            is(TCAMCommands.TCAM_STATUS) {
              state := s_status
              op_counter := 0.U
            }
            is(TCAMCommands.TCAM_CONFIG) {
              // For now, config is a no-op, just return success
              state := s_resp
              result := 1.U
            }
          }
        }
      }
      
      is(s_write) {
        // Write operation: rs1 = data, rs2 = address
        when(op_counter === 0.U) {
          tcam.io.in_csb := false.B
          tcam.io.in_web := false.B
          tcam.io.in_wmask := 0xF.U
          tcam.io.in_addr := rs2_data(27, 0)
          tcam.io.in_wdata := rs1_data
          op_counter := 1.U
        }.elsewhen(op_counter === 1.U) {
          // Hold signals for one more cycle
          tcam.io.in_csb := false.B
          tcam.io.in_web := false.B
          tcam.io.in_wmask := 0xF.U
          tcam.io.in_addr := rs2_data(27, 0)
          tcam.io.in_wdata := rs1_data
          op_counter := 2.U
        }.otherwise {
          // Write complete
          state := s_resp
          result := 0.U  // Success
        }
      }
      
      is(s_search) {
        // Search operation: rs1 = query data
        when(op_counter === 0.U) {
          tcam.io.in_csb := false.B
          tcam.io.in_web := true.B  // Read mode
          tcam.io.in_wmask := 0.U
          tcam.io.in_addr := rs1_data(27, 0)
          op_counter := 1.U
        }.elsewhen(op_counter === 1.U) {
          // Hold signals for one more cycle
          tcam.io.in_csb := false.B
          tcam.io.in_web := true.B
          tcam.io.in_wmask := 0.U
          tcam.io.in_addr := rs1_data(27, 0)
          op_counter := 2.U
        }.otherwise {
          // Capture result
          result := tcam.io.out_pma
          state := s_resp
        }
      }
      
      is(s_status) {
        // Status operation: return current TCAM output
        result := tcam.io.out_pma
        state := s_resp
      }
      
      is(s_resp) {
        // Send response if rd is valid
        when(cmd_reg.inst.xd) {
          io.resp.valid := true.B
          when(io.resp.ready) {
            busy := false.B
            state := s_idle
          }
        }.otherwise {
          // No response needed
          busy := false.B
          state := s_idle
        }
      }
    }
    
    private def log2Ceil(x: Int): Int = {
      if (x <= 1) 1 else 32 - Integer.numberOfLeadingZeros(x - 1)
    }
  }
}

// Config fragments for TCAM RoCC integration
class WithTCAMRoCC(params: TCAMParams = TCAMParams(), opcodes: OpcodeSet = OpcodeSet.custom0) extends Config((site, here, up) => {
  case BuildRoCC => up(BuildRoCC) ++ Seq((p: Parameters) => {
    val tcam = LazyModule(new TCAMRoCC(opcodes, params)(p))
    tcam
  })
  case TCAMRoCCKey => Some(params)
})

// Specific config for 64x28 TCAM RoCC
class WithTCAMRoCC64x28(opcodes: OpcodeSet = OpcodeSet.custom0) extends Config(
  new WithTCAMRoCC(TCAMParams(
    width = 32,
    keyWidth = 28,
    entries = 64,
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
  ), opcodes)
)

// Config for 32x28 TCAM RoCC
class WithTCAMRoCC32x28(opcodes: OpcodeSet = OpcodeSet.custom1) extends Config(
  new WithTCAMRoCC(TCAMParams(
    width = 32,
    keyWidth = 28,
    entries = 32,
    sramDepth = 256,
    sramWidth = 32,
    priorityEncoder = true,
    address = 0x4000,
    useAXI4 = false,
    tableConfig = Some(TCAMTableConfig(
      queryStrLen = 28,
      subStrLen = 7,
      totalSubStr = 4,
      potMatchAddr = 32
    ))
  ), opcodes)
)