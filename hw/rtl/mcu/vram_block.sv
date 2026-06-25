//------------------------------------------------------------------------------
// (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "defs.svh"

module vram_block #(
  parameter WORD_ADDRESS_WIDTH = 8    // 1K (256 x 32-bit words, byte-wide lanes)
)(
  // Port A - CPU (read-write)
  input  `VAR logic                          clk_a_i,
  input  `VAR logic                          we_a_i,
  input  `VAR logic [WORD_ADDRESS_WIDTH-1:0] addr_a_i,
  input  `VAR logic [7:0]                    data_a_i,
  output       logic [7:0]                   data_a_o,

  // Port B - pipeline (read-only)
  input  `VAR logic                          clk_b_i,
  input  `VAR logic [WORD_ADDRESS_WIDTH-1:0] addr_b_i,
  output       logic [7:0]                   data_b_o
);

logic [7:0] mem [1 << WORD_ADDRESS_WIDTH];

// Port A — synchronous read-write
always_ff @(posedge clk_a_i) begin
  if (we_a_i) mem[addr_a_i] <= data_a_i;
  data_a_o <= mem[addr_a_i];
end

// Port B — synchronous read-only
always_ff @(posedge clk_b_i)
  data_b_o <= mem[addr_b_i];

endmodule

`default_nettype wire