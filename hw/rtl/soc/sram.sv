//------------------------------------------------------------------------------
// Estella (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "defs.svh"


module sram #(
  parameter WORD_ADDRESS_WIDTH,
  parameter B0_MEM_FILE,
  parameter B1_MEM_FILE,
  parameter B2_MEM_FILE,
  parameter B3_MEM_FILE

)
(
  input `VAR logic        clk_i,
  input `VAR logic        rst_ni,
  input `VAR logic        select_i,

   // Native memory interface.
  output     logic        mem_ready_o,    // Memory access complete.
  input `VAR logic [3:0]  mem_wstrb_i,    // Data write strobe (byte enable)
  input `VAR logic [31:0] mem_addr_i,     // Memory address
  input `VAR logic [31:0] mem_wdata_i,    // Write data
  output     logic [31:0] mem_rdata_o     // Read data
);

//------------------------------------------------------------------------------
// Single-cycle delay - BSRAM has registered output
//------------------------------------------------------------------------------
always_ff @(posedge clk_i) begin
   if (!rst_ni)
    mem_ready_o <= 1'b0;
  else 
    mem_ready_o <= select_i;
end


//------------------------------------------------------------------------------
// Four byte-wide BSRAM blocks
//------------------------------------------------------------------------------

sram_block #(
  .MEM_FILE(B3_MEM_FILE),
  .WORD_ADDRESS_WIDTH(WORD_ADDRESS_WIDTH)
) u_b3 (
  .clk_i(clk_i),
  .rst_ni(rst_ni),
  .clk_en_i(select_i),
  .wrt_en_i(mem_wstrb_i[3]),
  .addr_i(mem_addr_i[WORD_ADDRESS_WIDTH+1:2]),
  .data_i(mem_wdata_i[31:24]),
  .data_o(mem_rdata_o[31:24])
);

sram_block #(
  .MEM_FILE(B2_MEM_FILE),
  .WORD_ADDRESS_WIDTH(WORD_ADDRESS_WIDTH)
) u_b2 (
  .clk_i(clk_i),
  .rst_ni(rst_ni),
  .clk_en_i(select_i),
  .wrt_en_i(mem_wstrb_i[2]),
  .addr_i(mem_addr_i[WORD_ADDRESS_WIDTH+1:2]),
  .data_i(mem_wdata_i[23:16]),
  .data_o(mem_rdata_o[23:16])
);

sram_block #(
  .MEM_FILE(B1_MEM_FILE),
  .WORD_ADDRESS_WIDTH(WORD_ADDRESS_WIDTH)
) u_b1 (
  .clk_i(clk_i),
  .rst_ni(rst_ni),
  .clk_en_i(select_i),
  .wrt_en_i(mem_wstrb_i[1]),
  .addr_i(mem_addr_i[WORD_ADDRESS_WIDTH+1:2]),
  .data_i(mem_wdata_i[15:8]),
  .data_o(mem_rdata_o[15:8])
);

sram_block #(
  .MEM_FILE(B0_MEM_FILE),
  .WORD_ADDRESS_WIDTH(WORD_ADDRESS_WIDTH)
) u_b0 (
  .clk_i(clk_i),
  .rst_ni(rst_ni),
  .clk_en_i(select_i),
  .wrt_en_i(mem_wstrb_i[0]),
  .addr_i(mem_addr_i[WORD_ADDRESS_WIDTH+1:2]),
  .data_i(mem_wdata_i[7:0]),
  .data_o(mem_rdata_o[7:0])
);

endmodule
`default_nettype wire
