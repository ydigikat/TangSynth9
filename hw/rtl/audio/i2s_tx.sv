//------------------------------------------------------------------------------
// (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "../defs.svh"

module i2s_tx (
  input  `VAR logic        clk_i,
  input  `VAR logic        rst_ni,

  output logic             req_o,
  input  `VAR logic [31:0] sample_i,

  output logic             aud_bclk_o,
  output logic             aud_lrclk_o,
  output logic             aud_sda_o
);

//------------------------------------------------------------------------------
// BCLK divider
// clk_i (24MHz) / BCLK_DIV = 3MHz 
// Fs = BCLK / 64 = 3MHz / 64 = 46875 Hz - acceptable for I2S and freq shift is
// corrected by FCW in oscillator.
//------------------------------------------------------------------------------
localparam int BCLK_DIV  = 16;
localparam int HALF_DIV  = BCLK_DIV / 2;

logic [$clog2(BCLK_DIV)-1:0] div_cnt;
logic bclk_r;
logic bclk_fe, bclk_re;

always_ff @(posedge clk_i) begin
  if (~rst_ni) begin
    div_cnt <= '0;
    bclk_r  <= 1'b0;
  end else begin
    if (div_cnt == BCLK_DIV - 1) begin
      div_cnt <= '0;
      bclk_r  <= 1'b0;
    end else begin
      div_cnt <= div_cnt + 1'b1;
      if (div_cnt == HALF_DIV - 1) bclk_r <= 1'b1;
    end
  end
end

// bclk_fe/re are combinatorial — one cycle ahead of the state update.
// bclk_fe_q is registered, so state registers have settled when it is seen.
assign bclk_fe = (div_cnt == BCLK_DIV - 1);
assign bclk_re = (div_cnt == HALF_DIV - 1);

logic bclk_fe_q;
always_ff @(posedge clk_i) begin
  if (~rst_ni) bclk_fe_q <= 1'b0;
  else         bclk_fe_q <= bclk_fe;
end

//------------------------------------------------------------------------------
// Registers  (serialiser outputs on BCLK falling edge)
//------------------------------------------------------------------------------
logic sda, sda_d, aud_lrclk, aud_lrclk_d;
logic [4:0] bit_cnt, bit_cnt_d;
logic [31:0] sample, sample_d;

always_ff @(posedge clk_i) begin
  if (~rst_ni) begin
    bit_cnt   <= 5'b0;
    aud_lrclk <= 1'b0;
    sda       <= 1'b0;
  end else if (bclk_fe) begin
    bit_cnt   <= bit_cnt_d;
    aud_lrclk <= aud_lrclk_d;
    sda       <= sda_d;
  end
end

always_ff @(posedge clk_i) begin
  if (~rst_ni) begin
    sample <= 32'b0;
  end else if (bclk_fe_q && !aud_lrclk && (bit_cnt == 0)) begin
    sample <= sample_i;
  end
end

//------------------------------------------------------------------------------
// Sample request pulse
//------------------------------------------------------------------------------
// Use bclk_fe_q so state (bit_cnt, aud_lrclk) has already settled
always_ff @(posedge clk_i) begin
  req_o <= bclk_fe_q && !aud_lrclk && (bit_cnt == 0);
end

//------------------------------------------------------------------------------
// Next state logic
//------------------------------------------------------------------------------
always_comb begin
  bit_cnt_d   = (bit_cnt == 15) ? 5'd0 : bit_cnt + 5'd1;
  aud_lrclk_d = (bit_cnt == 15) ? ~aud_lrclk : aud_lrclk;
  sda_d       = aud_lrclk ? sample[15-bit_cnt] : sample[31-bit_cnt];
end

//------------------------------------------------------------------------------
// Output logic
//------------------------------------------------------------------------------
assign aud_lrclk_o = aud_lrclk;
assign aud_bclk_o  = bclk_r;
assign aud_sda_o   = sda;

endmodule