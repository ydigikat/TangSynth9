//------------------------------------------------------------------------------
// Estella (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "defs.svh"

module top (
    input `VAR  logic       clk_i,  
    input `VAR  logic       rst_btn_ni,
//    input `VAR  logic       midi_i,    
    

    // IO lines
    output      logic       ftdi_o,      
    output      logic[4:0]  led_o,    
    output      logic       led_trap_o,    

    // I2S
    output      logic       i2s_bclk_o,
    output      logic       i2s_lrclk_o,
    output      logic       i2s_sda_o,

    // Debug output and triggers
    output      logic[15:0] d_o
);

  //------------------------------------------------------------------------------
  // Clock and reset generation.
  //------------------------------------------------------------------------------
  logic clk, rst_n;

  clock_gen cg (
      .clk_i(clk_i),        
      .rst_btn_ni(rst_btn_ni),   
      .clk_o(clk),          
      .rst_no(rst_n)      
  );

  //------------------------------------------------------------------------------
  // MCU (SOC)
  //------------------------------------------------------------------------------
  logic trap, trace, audio_irq, vram_valid, pipe_update;
  logic[15:0] gpo, debug;
  logic[7:0] vram_addr;
  logic[31:0] vram_data;
  
  soc #(             
     .B0_MEM_FILE(`B0_MEM_FILE),
     .B1_MEM_FILE(`B1_MEM_FILE),
     .B2_MEM_FILE(`B2_MEM_FILE),
     .B3_MEM_FILE(`B3_MEM_FILE)
  )
  u_soc (
    .clk_i(clk),
    .rst_ni(rst_n),
    .aud_irq_i(audio_irq),    
    .pipe_vram_addr_i(vram_addr),
    .pipe_vram_data_o(vram_data),
    .pipe_vram_valid_o(vram_valid),
    .pipe_vram_update_o(pipe_update),
    .gpo_o(gpo),
    .trap_o(trap),
    .trace_o(trace),
    .debug_o(debug)      
  );

  //------------------------------------------------------------------------------
  // Audio Pipeline
  //------------------------------------------------------------------------------  
  logic bclk,lrclk,sda;

  aud_pipeline u_aud_pipeline
  (
    .clk_i(clk),
    .rst_ni(rst_n),
    .irq_o(audio_irq),
    .aud_bclk_o(bclk),
    .aud_lrclk_o(lrclk),
    .aud_sda_o(sda),
    .vram_addr_o(vram_addr),
    .vram_data_i(vram_data),
    .vram_valid_i(vram_valid),
    .vram_update_i(pipe_update)
  );  
  

  //------------------------------------------------------------------------------
  // Outputs
  //------------------------------------------------------------------------------
  assign led_trap_o = ~trap;      // Processor trap indicator LED (active low)
  assign led_o[4:0] = ~gpo[4:0];  // First 5 gpo pins go to LEDs (active low)
  assign ftdi_o = trace;          // Serial trace output
  assign d_o = debug;             // Logic analyser test pins

  // assign d_o[0] = audio_irq;

  assign i2s_bclk_o = bclk;
  assign i2s_lrclk_o = lrclk;
  assign i2s_sda_o = sda;

  
  endmodule


`default_nettype wire
