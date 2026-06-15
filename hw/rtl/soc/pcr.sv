//------------------------------------------------------------------------------
// Estella (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "defs.svh"

module pcr(
  input `VAR  logic         clk_i,
  input `VAR  logic         rst_ni,

  output      logic         vram_update_o,   

  input  `VAR logic         select_i,
  output       logic        mem_ready_o,
  input  `VAR logic [3:0]   mem_wstrb_i,
  input  `VAR logic [31:0]  mem_addr_i,
  input  `VAR logic [31:0]  mem_wdata_i,
  output       logic [31:0] mem_rdata_o
);

//------------------------------------------------------------------------------
// Register map
//------------------------------------------------------------------------------
localparam CR = 4'h00;                // Control register
localparam CR_VRAM_UPDATE = 'h0;      // [0] : Update pipeline form VRAM

//------------------------------------------------------------------------------
// Pipeline Control Register (PCR) Operations
//------------------------------------------------------------------------------
logic wr_cr;
assign wr_cr = (|mem_wstrb_i && select_i && mem_addr_i[5:2] == CR);

//------------------------------------------------------------------------------
// Registers
//------------------------------------------------------------------------------
logic [31:0] cr, cr_next;

always_ff @(posedge clk_i) begin
  if (!rst_ni) cr <= 1'b0;
  else  cr <=  cr_next;      
end

//------------------------------------------------------------------------------
// Next state logic
//------------------------------------------------------------------------------
always_comb begin
  cr_next = cr;

  if(wr_cr) begin
    cr_next = mem_wdata_i;    
  end  
end

//------------------------------------------------------------------------------
// Output logic
//------------------------------------------------------------------------------
assign vram_update_o = cr[CR_VRAM_UPDATE];

assign mem_ready_o = select_i;  

assign mem_rdata_o = 32'b0;    // Always returns zeroes, MCU does not need read

endmodule


`default_nettype wire