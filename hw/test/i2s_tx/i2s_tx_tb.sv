//------------------------------------------------------------------------------
// Jason Wilden 2025
//------------------------------------------------------------------------------
// 1. Verify LRCLK
// 2. Verify_sample_req 
// 3. Verify Frame
//------------------------------------------------------------------------------

`default_nettype none
`timescale 1ns / 10ps

module i2s_tx_tb ();

  //------------------------------------------------------------------------------
  // Test control counts
  //------------------------------------------------------------------------------
  integer test_failures = 0;
  integer test_count    = 0;
  time    req_pulse_time = 0;

  //------------------------------------------------------------------------------
  // Clock and reset — 24MHz, period 41.667ns
  //------------------------------------------------------------------------------
  logic clk = 0;
  always #20.833 clk = ~clk;

  logic rst_n;
  initial begin
    rst_n = 0;
    #200;
    rst_n = 1;
  end

  //------------------------------------------------------------------------------
  // Unit under test
  //------------------------------------------------------------------------------
  logic aud_bclk, aud_lrclk, aud_sda, req;
  logic [31:0] sample;

  i2s_tx uut (
      .clk_i      (clk),
      .rst_ni     (rst_n),
      .req_o      (req),
      .sample_i   (sample),
      .aud_bclk_o (aud_bclk),
      .aud_lrclk_o(aud_lrclk),
      .aud_sda_o  (aud_sda)
  );

  //------------------------------------------------------------------------------
  // Monitor req timing for verify_sample_req
  //------------------------------------------------------------------------------
  always @(posedge clk) begin
    if (req) req_pulse_time = $time;
  end

  //------------------------------------------------------------------------------
  // Tests
  //------------------------------------------------------------------------------

  // 1. Verify LRCLK
  task automatic verify_lrclk;
    integer bclk_edge_count = 0;

    @(posedge aud_lrclk);
    #1;

    repeat (32) begin
      @(negedge aud_bclk);
      bclk_edge_count += 1;
    end

    #1;

    if (aud_lrclk !== 1'b1) begin
      $display("FAIL: verify_lrclk, not aligned after 32 clocks");
      test_failures += 1;
    end

    if (bclk_edge_count != 32) begin
      $display("FAIL: verify_lrclk, expected 32 clock edges, got %0d", bclk_edge_count);
      test_failures += 1;
    end
    test_count++;
  endtask

  
  // 2. Verify_sample_req 
  task automatic verify_sample_req;
    integer bclk_edge_count = 0;
    time    end_time;

    @(negedge aud_lrclk);
    #1;

    repeat (2) @(posedge clk);
    #1;

    req_pulse_time = 0;

    repeat (32) begin
      @(negedge aud_bclk);
      bclk_edge_count += 1;
    end

    end_time = $time;

    // Wait > one BCLK half-period (24MHz/16 -> ~667ns half-period) for req to register
    #800;

    if (req_pulse_time == 0 || req_pulse_time < end_time) begin
      $display("FAIL: verify_sample_req, pulse not at expected time");
      test_failures += 1;
    end
    test_count++;
  endtask


  // 3. Verify Frame
  task automatic verify_frame(logic [31:0] frame_to_verify, logic [31:0] next_sample);
    logic   failed = 0;
    integer i      = 31;

    // Wait for req pulse 
    wait (req);

    // Step past req before driving next frame.
    @(posedge clk);
    #1;
    sample = next_sample;

    // 32 bits shifting out for this frame.
    repeat (32) begin
      @(negedge aud_bclk);
      #1;
      if (aud_sda !== frame_to_verify[i]) begin
        failed = 1'b1;
        $display("FAIL: Bit: %0d | expecting %0x, got %0x", i,
                 frame_to_verify[i], aud_sda);
      end
      i--;
    end

    if (failed) test_failures++;
    test_count++;
  endtask

  //------------------------------------------------------------------------------
  // Test executor
  //------------------------------------------------------------------------------
  initial begin
    $dumpfile("i2s_tx_tb.fst");
    $dumpvars(0, i2s_tx_tb);
    $display("TESTBENCH: i2s_tx_tb");
    
    sample = 32'h0000_0001;

    wait (rst_n);
    repeat (2) @(negedge aud_lrclk);

    verify_lrclk();
    verify_sample_req();
    verify_frame(32'h0000_0001,  32'h8000_8000);
    verify_frame(32'h8000_8000,  32'h0000_0000);
    verify_frame(32'h0000_0000,  32'hFFFF_FFFF);
    verify_frame(32'hFFFF_FFFF,  32'hCAFE_FEED);
    verify_frame(32'hCAFE_FEED,  32'hCAFE_FEED);

    for (integer i = 0; i < 25; i++) begin
      verify_frame(32'hCAFE_FEED, 32'hCAFE_FEED);
    end

    repeat (10) @(posedge clk);

    if (test_failures == 0) $display("PASS: All %0d tests passed.", test_count);
    else $fatal(1, "FAIL: i2s_tx_tb. %0d test(s) failed.", test_failures);

    $finish;
  end

endmodule