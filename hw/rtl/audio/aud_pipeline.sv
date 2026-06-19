//------------------------------------------------------------------------------
// (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "../defs.svh"

module aud_pipeline(
  input `VAR  logic   clk_i,
  input `VAR  logic   rst_ni,

  // Voice RAM
  output      logic[7:0]  vram_addr_o,    // Address of data to fetch
  input `VAR  logic[31:0] vram_data_i,    // VRAM data 
  input `VAR  logic       vram_valid_i,   // CPU is not writing
  input `VAR  logic       vram_update_i,  // Update is required on next sample
  
  // Audio interrupt out
  output      logic   irq_o,

  // I2S out
  output      logic   aud_bclk_o,
  output      logic   aud_lrclk_o,
  output      logic   aud_sda_o
);

logic update_reqd;


//------------------------------------------------------------------------------
// Audio pipeline fans out into 4 voices
//------------------------------------------------------------------------------





// Voice processing here x 4


//------------------------------------------------------------------------------
// Voices mixed back into single output and scaled for I2S
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// I2S peripheral
//------------------------------------------------------------------------------
logic bclk, lrclk, sda;

i2s_tx it
(
  .clk_i(clk_i),
  .rst_ni(rst_ni),
  .sample_i({left,right}),
  .req_o(req),
  .aud_bclk_o(bclk),
  .aud_lrclk_o(lrclk),
  .aud_sda_o(sda)    
);

//------------------------------------------------------------------------------
// Test sawtooth generator
//------------------------------------------------------------------------------
logic [15:0] left;
logic [15:0] right;
logic req;


test_tone tt
(
  .clk_i(clk_i),
  .rst_ni(rst_ni),
  .fcw_i(16'd615),
  .sample_req_i(req),
  .sample_o({left,right})
);


//------------------------------------------------------------------------------
// Outputs
//------------------------------------------------------------------------------
assign aud_bclk_o = bclk;
assign aud_lrclk_o = lrclk;
assign aud_sda_o = sda;
assign irq_o = req;

endmodule

`default_nettype  wire
