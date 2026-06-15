//------------------------------------------------------------------------------
// Estella (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "defs.svh"

module vram(
  input `VAR  logic         clk_i,
  input `VAR  logic         rst_ni,

  // CPU port (native memory access)
  input  `VAR logic         cpu_select_i,
  output       logic        cpu_ready_o,
  input  `VAR logic [3:0]   cpu_wstrb_i,
  input  `VAR logic [31:0]  cpu_addr_i,
  input  `VAR logic [31:0]  cpu_wdata_i,
  output       logic [31:0] cpu_rdata_o,


  // Audio pipeline port (read only)
  input  `VAR logic [WORD_ADDRESS_WIDTH-1:0] pipe_addr_i,
  output       logic [31:0]                  pipe_data_o,
  output       logic                         pipe_valid_o   
);

localparam WORD_ADDRESS_WIDTH=8;      // 1K memory size

//------------------------------------------------------------------------------
// Address mux: CPU address when cpu_active, pipeline address otherwise
//------------------------------------------------------------------------------
logic [WORD_ADDRESS_WIDTH-1:0] addr;
logic [3:0]  wstrb;
logic [31:0] wdata;
logic        clk_en;

// CPU takes priority but pipeline reads should be supressed during CPU activity
// anyway.
logic cpu_active;
assign cpu_active = cpu_select_i;

// Pipeline is read only.
assign addr = cpu_active ? cpu_addr_i[WORD_ADDRESS_WIDTH+1:2] : pipe_addr_i;
assign wstrb = cpu_active ? cpu_wstrb_i : 4'b0; 
assign wdata = cpu_wdata_i;
assign clk_en = 1'b1;

logic[31:0] bsram_rdata;

//------------------------------------------------------------------------------
// Four byte-wide BSRAM blocks
//------------------------------------------------------------------------------

sram_block #(
  .MEM_FILE(""),
  .WORD_ADDRESS_WIDTH(WORD_ADDRESS_WIDTH)
) u_b3 (
  .clk_i(clk_i), .rst_ni(rst_ni),
  .clk_en_i(clk_en),
  .wrt_en_i(wstrb[3]),
  .addr_i(addr),
  .data_i(wdata[31:24]),
  .data_o(bsram_rdata[31:24])
);

sram_block #(
  .MEM_FILE(""),
  .WORD_ADDRESS_WIDTH(WORD_ADDRESS_WIDTH)
) u_b2 (
  .clk_i(clk_i), .rst_ni(rst_ni),
  .clk_en_i(clk_en),
  .wrt_en_i(wstrb[2]),
  .addr_i(addr),
  .data_i(wdata[23:16]),
  .data_o(bsram_rdata[23:16])
);

sram_block #(
  .MEM_FILE(""),
  .WORD_ADDRESS_WIDTH(WORD_ADDRESS_WIDTH)
) u_b1 (
  .clk_i(clk_i), .rst_ni(rst_ni),
  .clk_en_i(clk_en),
  .wrt_en_i(wstrb[1]),
  .addr_i(addr),
  .data_i(wdata[15:8]),
  .data_o(bsram_rdata[15:8])
);

sram_block #(
  .MEM_FILE(""),
  .WORD_ADDRESS_WIDTH(WORD_ADDRESS_WIDTH)
) u_b0 (
  .clk_i(clk_i), .rst_ni(rst_ni),
  .clk_en_i(clk_en),
  .wrt_en_i(wstrb[0]),
  .addr_i(addr),
  .data_i(wdata[7:0]),
  .data_o(bsram_rdata[7:0])
);

//------------------------------------------------------------------------------
// CPU ready - single cycle delay
//------------------------------------------------------------------------------
always_ff @(posedge clk_i) begin
  if (~rst_ni) cpu_ready_o <= 1'b0;
  else         cpu_ready_o <= cpu_select_i;
end

logic cpu_active_q;
always_ff @(posedge clk_i) begin
  if(~rst_ni) cpu_active_q <= 1'b0;
  else        cpu_active_q <= cpu_active;
end

//------------------------------------------------------------------------------
// Outputs
//------------------------------------------------------------------------------
assign cpu_rdata_o  = cpu_active_q  ? bsram_rdata : 32'h0;

assign pipe_data_o  = ~cpu_active_q ? bsram_rdata : 32'h0;
assign pipe_valid_o = ~cpu_active_q;    

endmodule


`default_nettype wire