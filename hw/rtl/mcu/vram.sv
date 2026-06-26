//------------------------------------------------------------------------------
// (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "../defs.svh"

module vram(
  input `VAR  logic         clk_i,
  input `VAR  logic         rst_ni,

  // CPU port (native memory interface — read-write)
  input  `VAR logic         cpu_select_i,
  output       logic        cpu_ready_o,
  input  `VAR logic [3:0]   cpu_wstrb_i,
  input  `VAR logic [31:0]  cpu_addr_i,
  input  `VAR logic [31:0]  cpu_wdata_i,
  output       logic [31:0] cpu_rdata_o,

  // Audio pipeline port (read-only)
  input  `VAR logic [WORD_ADDRESS_WIDTH-1:0] pipe_addr_i,
  output       logic [31:0]                  pipe_data_o,
  output       logic                         pipe_valid_o
);

localparam WORD_ADDRESS_WIDTH = 8;    // 1K memory (256 x 32-bit words)

logic [WORD_ADDRESS_WIDTH-1:0] cpu_word_addr;
assign cpu_word_addr = cpu_addr_i[WORD_ADDRESS_WIDTH+1:2];

//------------------------------------------------------------------------------
// Four byte-wide dual-port BSRAM blocks (one per byte lane)
//------------------------------------------------------------------------------

vram_block #(.WORD_ADDRESS_WIDTH(WORD_ADDRESS_WIDTH)) u_b3 (
  .clk_a_i  (clk_i),
  .we_a_i   (cpu_select_i & cpu_wstrb_i[3]),
  .addr_a_i (cpu_word_addr),
  .data_a_i (cpu_wdata_i[31:24]),
  .data_a_o (cpu_rdata_o[31:24]),
  .clk_b_i  (clk_i),
  .addr_b_i (pipe_addr_i),
  .data_b_o (pipe_data_o[31:24])
);

vram_block #(.WORD_ADDRESS_WIDTH(WORD_ADDRESS_WIDTH)) u_b2 (
  .clk_a_i  (clk_i),
  .we_a_i   (cpu_select_i & cpu_wstrb_i[2]),
  .addr_a_i (cpu_word_addr),
  .data_a_i (cpu_wdata_i[23:16]),
  .data_a_o (cpu_rdata_o[23:16]),
  .clk_b_i  (clk_i),
  .addr_b_i (pipe_addr_i),
  .data_b_o (pipe_data_o[23:16])
);

vram_block #(.WORD_ADDRESS_WIDTH(WORD_ADDRESS_WIDTH)) u_b1 (
  .clk_a_i  (clk_i),
  .we_a_i   (cpu_select_i & cpu_wstrb_i[1]),
  .addr_a_i (cpu_word_addr),
  .data_a_i (cpu_wdata_i[15:8]),
  .data_a_o (cpu_rdata_o[15:8]),
  .clk_b_i  (clk_i),
  .addr_b_i (pipe_addr_i),
  .data_b_o (pipe_data_o[15:8])
);

vram_block #(.WORD_ADDRESS_WIDTH(WORD_ADDRESS_WIDTH)) u_b0 (
  .clk_a_i  (clk_i),
  .we_a_i   (cpu_select_i & cpu_wstrb_i[0]),
  .addr_a_i (cpu_word_addr),
  .data_a_i (cpu_wdata_i[7:0]),
  .data_a_o (cpu_rdata_o[7:0]),
  .clk_b_i  (clk_i),
  .addr_b_i (pipe_addr_i),
  .data_b_o (pipe_data_o[7:0])
);

//------------------------------------------------------------------------------
// CPU ready — one-cycle delay matching BSRAM registered output latency
//------------------------------------------------------------------------------
always_ff @(posedge clk_i) begin
  if (~rst_ni) cpu_ready_o <= 1'b0;
  else         cpu_ready_o <= cpu_select_i;
end

//------------------------------------------------------------------------------
// Pipeline valid — one-cycle delay, unconditional (no suppression needed)
//------------------------------------------------------------------------------
always_ff @(posedge clk_i) begin
  if (~rst_ni) pipe_valid_o <= 1'b0;
  else         pipe_valid_o <= 1'b1;
end

endmodule

`default_nettype wire