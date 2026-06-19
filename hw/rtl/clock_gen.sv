//------------------------------------------------------------------------------
// (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none

module clock_gen (  
    input `VAR  logic clk_i,  
    input `VAR  logic rst_btn_ni,  
    output      logic clk_o,  
    output      logic rst_no    
);

/* verilator lint_off MODMISSING */
  rPLL #(
      .FCLKIN("27"),
      .DYN_IDIV_SEL("false"),
      .IDIV_SEL(8),
      .DYN_FBDIV_SEL("false"),
      .FBDIV_SEL(7),
      .DYN_ODIV_SEL("false"),
      .ODIV_SEL(32),
      .PSDA_SEL("0000"),
      .DYN_DA_EN("true"),
      .DUTYDA_SEL("1000"),
      .CLKOUT_FT_DIR(1),
      .CLKOUTP_FT_DIR(1),
      .CLKOUT_DLY_STEP(0),
      .CLKOUTP_DLY_STEP(0),
      .CLKFB_SEL("internal"),
      .CLKOUT_BYPASS("false"),
      .CLKOUTP_BYPASS("false"),
      .CLKOUTD_BYPASS("false"),      
      .DYN_SDIV_SEL(2),
      .CLKOUTD_SRC("CLKOUT"),
      .CLKOUTD3_SRC("CLKOUT"),
      .DEVICE("GW1NR-9C")
  ) u_pll (
      .CLKIN(clk_i),
      .CLKOUT(clk),           
      .LOCK(pll_locked),
      .CLKOUTP(),
      .CLKOUTD(),
      .CLKOUTD3(),

      // Unused      
      .RESET(1'b0),
      .RESET_P(1'b0),
      .CLKFB(1'b0),      
      .FBDSEL({1'b0, 1'b0, 1'b0, 1'b0, 1'b0, 1'b0}),
      .IDSEL({1'b0, 1'b0, 1'b0, 1'b0, 1'b0, 1'b0}),
      .ODSEL({1'b0, 1'b0, 1'b0, 1'b0, 1'b0, 1'b0}),
      .PSDA({1'b0, 1'b0, 1'b0, 1'b0}),
      .DUTYDA({1'b0, 1'b0, 1'b0, 1'b0}),
      .FDLY({1'b0, 1'b0, 1'b0, 1'b0})
  );
  /* verilator lint_on MODMISSING */

  logic clk, pll_locked;

  //------------------------------------------------------------------------------
  // State registers
  //------------------------------------------------------------------------------
  logic [3:0] rst, rst_d;

  always_ff @(posedge clk or negedge rst_btn_ni) begin
    if (!rst_btn_ni) begin
      rst <= 4'b0;      
    end else begin
      rst <= rst_d;     
             
    end
  end

  //------------------------------------------------------------------------------
  // Next state logic - hold reset for 4 cycles, the picorv32 CPU wants reset 
  // held for 2-3 cycles according to doco.
  //------------------------------------------------------------------------------
  always_comb begin
    rst_d = 4'b0;    
    if (pll_locked) begin
      rst_d = {rst[2:0], 1'b1};            
    end
  end

  //------------------------------------------------------------------------------
  // Output logic
  //------------------------------------------------------------------------------
  assign clk_o = clk;
  assign rst_no = rst[3];  

endmodule

`default_nettype wire

