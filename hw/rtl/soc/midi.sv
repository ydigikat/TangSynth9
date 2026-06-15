//------------------------------------------------------------------------------
// (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "../../defs.svh"


module midi (
  input `VAR  logic        clk_i,
  input `VAR  logic        rst_ni,
  input `VAR  logic        clk_mcu_i,
  input `VAR  logic        rst_mcu_ni,
  input `VAR  logic        select_i,   
  input `VAR  logic        midi_i,       

  output      logic        mem_ready_o,    
  input `VAR  logic [3:0]  mem_wstrb_i,    
  input `VAR  logic [31:0] mem_addr_i,     
  input `VAR  logic [31:0] mem_wdata_i,    
  output      logic [31:0] mem_rdata_o     

);
endmodule


`default_nettype wire
