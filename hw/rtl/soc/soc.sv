//------------------------------------------------------------------------------
// (c) Jason Wilden 2026
//------------------------------------------------------------------------------

`default_nettype none
`include "defs.svh"

module soc #(parameter string B0_MEM_FILE,    // ROM byte files
                       string B1_MEM_FILE,
                       string B2_MEM_FILE,
                       string B3_MEM_FILE)
(
  input `VAR logic        clk_i,       
  input `VAR logic        rst_ni,

  // Audio interrupt
  input `VAR logic        aud_irq_i,  

  // Pipeline to SOC integration
  input  `VAR logic[7:0]  pipe_vram_addr_i,
  output      logic[31:0] pipe_vram_data_o,
  output      logic       pipe_vram_valid_o,
  output      logic       pipe_vram_update_o,


  // Module IO 
  output     logic[15:0]  gpo_o,  
  output     logic        trap_o,
  output     logic        trace_o,
  output     logic[15:0]  debug_o,
  output     logic        midi_o,
  input `VAR logic        midi_i


);

//------------------------------------------------------------------------------
// MCU Configuration
//------------------------------------------------------------------------------  
localparam int IRQ_MASK = 32'hFFFF_FFE0;              // Only bits 4:0 used.
localparam int ADDR_WIDTH = 'd12;                     // Word address size
localparam int SRAM_SIZE = (1 << (ADDR_WIDTH + 2));   // SRAM in bytes (16K)

//------------------------------------------------------------------------------
// Address Map.  
// RAM starts at 0x0 and the size is based on the word address width.
// VRAM (voice ram) is at 0x00010000.
// Peripherals start at 0x80000000 and each provdes 0x40 words of reg space.
//------------------------------------------------------------------------------
localparam int SRAM_ADDR = 32'h0000_0000;
localparam int VRAM_ADDR = 32'h0001_0000;
localparam int GPO_ADDR = 32'h8000_0000;
localparam int TRACE_ADDR =  32'h8000_0040;
localparam int PCR_ADDR =  32'h8000_0080;
localparam int MIDI_ADDR = 32'h8000_00C0;

//------------------------------------------------------------------------------
// Native memory interface signals
//------------------------------------------------------------------------------
logic        mem_valid;   // High when core access memory (fetch, load or store)
logic        mem_instr;   // High if fetching an instruction, low for data.
logic        mem_ready;   // Ack from memory/peripheral that access is complete.
logic [31:0] mem_addr;    // 32-bit byte address of memory accessed.
logic [31:0] mem_wdata;   // 32-bit data to be written;
logic [3:0]  mem_wstrb;   // Indicates active bytes in write data 0-3, 0+1, 2+3
logic [31:0] mem_rdata;   // 32-bit data read.
logic        trap;


//------------------------------------------------------------------------------
// Interrupts
//------------------------------------------------------------------------------
logic[31:0] irq;
assign irq = 0;
assign irq[3] = aud_irq_i;

//------------------------------------------------------------------------------
// PicoRV32 RISC V soft core processor 
//------------------------------------------------------------------------------


picorv32 #(

  .ENABLE_DIV(1),               // Enable hardware div,mul and shift
  .ENABLE_MUL(1),               // these need the rv32im architecture flag
  .BARREL_SHIFTER(1),           // on compiler/linker to support them.

  .ENABLE_IRQ(1),               // Interupts off for time-being.
  .ENABLE_IRQ_TIMER(0),         // Don't need the timer.
  .ENABLE_IRQ_QREGS(1),         // Shadow interrupt registers
  .MASKED_IRQ(IRQ_MASK),        // Only irqs 0-4    

  .CATCH_ILLINSN(1),            // Set these low for traps to go to irq handler
	.CATCH_MISALIGN(1)            // for debug. Otherwise trap_o is fired (led).

) mcu (
  .clk(clk_i),
  .resetn(rst_ni),
  .trap(trap),

  .mem_valid(mem_valid),
  .mem_instr(mem_instr),
  .mem_ready(mem_ready),
  .mem_addr(mem_addr),
  .mem_wdata(mem_wdata),
  .mem_wstrb(mem_wstrb),
  .mem_rdata(mem_rdata),
  .irq(irq),
  .eoi(),

   // Unused (reduce warning noise)
  .trace_valid(),
  .trace_data(),
  .pcpi_valid(),
  .pcpi_insn(),
  .pcpi_rs1(),
  .pcpi_rs2(),// 8000_00
  .pcpi_wr(1'b0),
  .pcpi_rd(32'h0),
  .pcpi_wait(1'b0),
  .pcpi_ready(1'b0)
);  



//------------------------------------------------------------------------------
// Address decoder 
//------------------------------------------------------------------------------
logic sram_sel, tick_sel, gpo_sel, trace_sel, vram_sel, pcr_sel, midi_sel;

assign sram_sel = mem_valid && (mem_addr  < SRAM_SIZE);  
assign vram_sel = mem_valid && (mem_addr >= VRAM_ADDR && mem_addr < GPO_ADDR);
assign gpo_sel =  mem_valid && (mem_addr >= GPO_ADDR && mem_addr < GPO_ADDR + 'h40);
assign trace_sel =  mem_valid && (mem_addr >= TRACE_ADDR && mem_addr < TRACE_ADDR + 'h40);
assign pcr_sel = mem_valid && (mem_addr >= PCR_ADDR && mem_addr < PCR_ADDR + 'h40);
assign midi_sel = mem_valid && (mem_addr >= MIDI_ADDR && mem_addr < MIDI_ADDR + 'h40);

//------------------------------------------------------------------------------
// Data bus selector
//------------------------------------------------------------------------------
assign mem_ready = mem_valid && (sram_rdy | gpo_rdy | trace_rdy | vram_rdy | pcr_rdy | midi_rdy); 

assign mem_rdata = sram_sel ? sram_rdata :                    
                   vram_sel ? vram_rdata : 
                   gpo_sel ? gpo_rdata : 
                   pcr_sel ? pcr_rdata :
                   midi_sel ? midi_rdata :
                   trace_sel ? trace_rdata : 
                   32'h0;


//------------------------------------------------------------------------------
// SRAM module. Any changes to sizing here require the same changes to be made
// to the firmware linker script.
//------------------------------------------------------------------------------
logic sram_rdy;
logic [31:0] sram_rdata;

sram #(
  .WORD_ADDRESS_WIDTH(ADDR_WIDTH),
  .B0_MEM_FILE(B0_MEM_FILE),
  .B1_MEM_FILE(B1_MEM_FILE),
  .B2_MEM_FILE(B2_MEM_FILE),
  .B3_MEM_FILE(B3_MEM_FILE)
) u_sram (
  .clk_i(clk_i),
  .rst_ni(rst_ni),
  .select_i(sram_sel),
  .mem_ready_o(sram_rdy),
  .mem_addr_i(mem_addr),   
  .mem_wstrb_i(mem_wstrb),
  .mem_wdata_i(mem_wdata),
  .mem_rdata_o(sram_rdata)
);

//------------------------------------------------------------------------------
// Voice RAM module.
//------------------------------------------------------------------------------
logic vram_rdy,pipe_vram_valid;
logic [31:0] vram_rdata;
logic [31:0]pipe_vram_data;

vram u_vram(
  .clk_i(clk_i),
  .rst_ni(rst_ni),

  // CPU interface to VRAM
  .cpu_select_i(vram_sel),
  .cpu_ready_o(vram_rdy),
  .cpu_addr_i(mem_addr),
  .cpu_wstrb_i(mem_wstrb),
  .cpu_wdata_i(mem_wdata),
  .cpu_rdata_o(vram_rdata),

  // Pipeline interface to VRAM
  .pipe_addr_i(pipe_vram_addr_i),
  .pipe_data_o(pipe_vram_data),
  .pipe_valid_o(pipe_vram_valid)
);

//------------------------------------------------------------------------------
// GPO (general purpose output) module, the pins these outputs are assigned to 
// is set in the top module, the firmware can assign a value but not the pins to
// which they are assigned.
//------------------------------------------------------------------------------
logic gpo_rdy;
logic [31:0] gpo_rdata;
logic [15:0] gpo;

gpo u_gpo(  
  .clk_i(clk_i),
  .rst_ni(rst_ni),  
  .select_i(gpo_sel),
  .gpo_o(gpo),
  .mem_ready_o(gpo_rdy),
  .mem_addr_i(mem_addr),
  .mem_wstrb_i(mem_wstrb),
  .mem_wdata_i(mem_wdata),
  .mem_rdata_o(gpo_rdata)  
);

//------------------------------------------------------------------------------
// Trace module.  This is a UART peripheral which is wired via FTDI to provide
//                printf tracing over USB. 
//------------------------------------------------------------------------------
logic trace_rdy, trace;
logic [31:0] trace_rdata;

trace u_trace(
  .clk_i(clk_i),
  .rst_ni(rst_ni),  
  .select_i(trace_sel),
  .trace_o(trace),  
  .mem_ready_o(trace_rdy),
  .mem_addr_i(mem_addr),
  .mem_wstrb_i(mem_wstrb),
  .mem_wdata_i(mem_wdata),
  .mem_rdata_o(trace_rdata)
);


//------------------------------------------------------------------------------
// General purpose control register module.  
// [0] = Pipeline to update from VRAM
// [15:1] unassigned.
//------------------------------------------------------------------------------
logic pcr_rdy, pipe_vram_update;
logic [31:0] pcr_rdata;

pcr u_pcr(
  .clk_i(clk_i),
  .rst_ni(rst_ni),
  .select_i(pcr_sel),
  .vram_update_o(pipe_vram_update),
  .mem_ready_o(pcr_rdy),
  .mem_addr_i(mem_addr),
  .mem_wstrb_i(mem_wstrb),
  .mem_wdata_i(mem_wdata),
  .mem_rdata_o(pcr_rdata)
);

//------------------------------------------------------------------------------
// MIDI in module - this is just the serial rx, no MIDI parsing done here.
//------------------------------------------------------------------------------
logic midi_rdy;
logic [31:0] midi_rdata;

midi u_midi(
  .clk_i(clk_i),
  .rst_ni(rst_ni),
  .select_i(midi_sel),
  .midi_i(midi_i),
  .mem_ready_o(midi_rdy),
  .mem_addr_i(mem_addr),
  .mem_wstrb_i(mem_wstrb),
  .mem_wdata_i(mem_wdata),
  .mem_rdata_o(midi_rdata)
);

//------------------------------------------------------------------------------
// Outputs
//------------------------------------------------------------------------------
assign pipe_vram_data_o = pipe_vram_data;
assign pipe_vram_valid_o = pipe_vram_valid;
assign pipe_vram_update_o = pipe_vram_update;
assign gpo_o = gpo;
assign trace_o = trace;
assign trap_o = trap;



// Debugging
assign debug_o[0] = irq[3];
assign debug_o[15:1] = 0;
// assign debug_o[1] = gpo_sel;
// assign debug_o[2] = tick_sel;
// assign debug_o[3] = trace_sel;
// assign debug_o[4] = trace_o;
// assign debug_o[5] = mem_instr;


endmodule
`default_nettype wire