//------------------------------------------------------------------------------
// (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "defs.svh"

module gpo
(
  input `VAR  logic        clk_i,
  input `VAR  logic        rst_ni,  

  input `VAR  logic        select_i,  
  output      logic[15:0]  gpo_o,     
  
  output      logic        mem_ready_o,    
  input `VAR  logic [3:0]  mem_wstrb_i,    
  input `VAR  logic [31:0] mem_addr_i,     
  input `VAR  logic [31:0] mem_wdata_i,    
  output      logic [31:0] mem_rdata_o      
);


//------------------------------------------------------------------------------
// Register map
//------------------------------------------------------------------------------
localparam BSR = 4'h00;               // [31:16] reset, [15:0] set

//------------------------------------------------------------------------------
// Operations
//------------------------------------------------------------------------------
logic wr_pins;
assign wr_pins = (|mem_wstrb_i && select_i && mem_addr_i[5:2] == BSR);

//------------------------------------------------------------------------------
// State registers
//------------------------------------------------------------------------------
logic [15:0] gpo, gpo_next;           

always_ff @(posedge clk_i) begin
  if (!rst_ni) gpo <= 16'b0;
  else  gpo <=  gpo_next;      
end

//------------------------------------------------------------------------------
// Next state logic
//------------------------------------------------------------------------------
always_comb begin
  gpo_next = gpo;

  if(wr_pins) begin
    gpo_next = gpo | mem_wdata_i[15:0];    // Sets
    gpo_next = gpo_next & ~mem_wdata_i[31:16];  // Clears
  end  
end

//------------------------------------------------------------------------------
// Output logic
//------------------------------------------------------------------------------
assign gpo_o = gpo;
assign mem_ready_o = select_i;  
assign mem_rdata_o = 32'b0;    // Always returns zeroes.

endmodule

`default_nettype wire