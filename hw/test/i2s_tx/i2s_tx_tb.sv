//------------------------------------------------------------------------------
// Jason Wilden 2025
//------------------------------------------------------------------------------
`default_nettype none
`timescale 1ns / 10ps

module i2s_tx_tb ();

  //------------------------------------------------------------------------------
  // Test control
  //------------------------------------------------------------------------------
  integer test_failures = 0;
  integer test_count    = 0;
  time    req_pulse_time = 0;

  //------------------------------------------------------------------------------
  // Clock and reset — 48MHz, period 20.833ns
  //------------------------------------------------------------------------------
  logic clk = 0;
  always #10.417 clk = ~clk;

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
  // Monitor — req is now a registered clk_i signal, sample on posedge
  //------------------------------------------------------------------------------
  always @(posedge clk) begin
    if (req) req_pulse_time = $time;
  end

  //------------------------------------------------------------------------------
  // Tests — unchanged: all operate on aud_bclk/aud_lrclk edges
  //------------------------------------------------------------------------------

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

    // Wait > one BCLK half-period (48MHz/32 -> ~333ns half-period) for req to register
    #400;

    if (req_pulse_time == 0 || req_pulse_time < end_time) begin
      $display("FAIL: verify_sample_req, pulse not at expected time");
      test_failures += 1;
    end
    test_count++;
  endtask

  task automatic verify_frame(logic [31:0] expected_result);
    logic   failed = 0;
    integer i      = 31;

    wait (req);
    sample = expected_result;

    repeat (32) begin
      @(negedge aud_bclk);
      #1;

      if (aud_sda != expected_result[i]) begin
        failed = 1'b1;
        $display("FAIL: Bit: %0d | expecting %0x, got %0x", i,
                 expected_result[i], aud_sda);
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

    sample = 0;

    wait (rst_n);
    repeat (2) @(negedge aud_lrclk);

    verify_lrclk();
    verify_sample_req();

    // verify_frame checks the frame that shifts out AFTER req captures sample_i.
    // So we prime sample with the first expected value, then each call loads the
    // *next* value while verifying the *current* one.
    sample = 32'b0000000000000001_0000000000000001;  // prime for first frame
    verify_frame(32'b0000000000000001_0000000000000001);
    verify_frame(32'b1000000000000000_1000000000000000);
    verify_frame(32'b0);
    verify_frame(32'hFFFF_FFFF);
    verify_frame(32'hCAFE_FEED);

    for (integer i = 0; i < 25; i++) begin
      verify_frame(sample);
    end

    repeat (10) @(posedge clk);

    if (test_failures == 0) $display("PASS: All %0d tests passed.", test_count);
    else $fatal(1, "FAIL: i2s_tx_tb. %0d test(s) failed.", test_failures);

    $finish;
  end

endmodule