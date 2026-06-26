//------------------------------------------------------------------------------
// Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`timescale 1ns / 10ps

module serial_rx_tb ();

//------------------------------------------------------------------------------
// Test control
//------------------------------------------------------------------------------
integer test_failures = 0;
integer test_count    = 0;

//------------------------------------------------------------------------------
// DUT parameters
// DVSR sets the oversampling divisor: sample_inc fires when sample_div == DVSR.
// One bit period = 16 sample_inc pulses, each DVSR+1 clocks wide.
// BIT_CLOCKS = 16 * (DVSR + 1).
//------------------------------------------------------------------------------
localparam logic [10:0] DVSR       = 11'd4;
localparam integer      BIT_CLOCKS = 16 * (DVSR + 1);

//------------------------------------------------------------------------------
// Clock and reset
//------------------------------------------------------------------------------
logic clk = 0;
always #5 clk = ~clk;

logic rst_n;
initial begin
  rst_n = 0;
  #50;
  rst_n = 1;
end

//------------------------------------------------------------------------------
// Unit under test
//------------------------------------------------------------------------------
logic        rx_i     = 1'b1;    // Idle high
logic [7:0]  rx_data;
logic        rx_valid;
logic        rx_error;

serial_rx uut (
  .clk_i     (clk),
  .rst_ni    (rst_n),
  .div_i     (DVSR),
  .rx_i      (rx_i),
  .rx_data_o (rx_data),
  .rx_valid_o(rx_valid),
  .rx_error_o(rx_error)
);

//------------------------------------------------------------------------------
// drive_byte — serialise one byte onto rx_i (LSB first, no framing error)
// UART frame: START(0) | D0..D7 | STOP(1)
//
// The double-flop sync in serial_rx adds 2 clk cycles of latency to rx_i.
// We drive rx_i combinatorially; the BIT_CLOCKS period is wide enough that
// the 2-cycle skew is irrelevant.
//------------------------------------------------------------------------------
task automatic drive_byte(logic [7:0] data);
  // Start bit
  @(posedge clk); #1;
  rx_i = 1'b0;
  repeat (BIT_CLOCKS) @(posedge clk);

  // Data bits — LSB first
  for (int i = 0; i < 8; i++) begin
    rx_i = data[i];
    repeat (BIT_CLOCKS) @(posedge clk);
  end

  // Stop bit
  rx_i = 1'b1;
  repeat (BIT_CLOCKS) @(posedge clk);
endtask

//------------------------------------------------------------------------------
// drive_framing_error — like drive_byte but holds rx_i low for the stop bit
//------------------------------------------------------------------------------
task automatic drive_framing_error(logic [7:0] data);
  @(posedge clk); #1;
  rx_i = 1'b0;
  repeat (BIT_CLOCKS) @(posedge clk);

  for (int i = 0; i < 8; i++) begin
    rx_i = data[i];
    repeat (BIT_CLOCKS) @(posedge clk);
  end

  // Stop bit held low — framing error
  rx_i = 1'b0;
  repeat (BIT_CLOCKS) @(posedge clk);

  // Return line to idle
  rx_i = 1'b1;
  repeat (BIT_CLOCKS) @(posedge clk);
endtask

//------------------------------------------------------------------------------
// Tests
//------------------------------------------------------------------------------
task automatic verify_reset_state;
  @(posedge clk); #1;

  if (rx_valid !== 1'b0 || rx_error !== 1'b0) begin
    $display("FAIL: verify_reset_state: rx_valid=%0b rx_error=%0b",
             rx_valid, rx_error);
    test_failures++;
  end
  test_count++;
endtask

task automatic verify_idle_line;
  // With rx_i held high, neither output should fire over several bit periods.
  logic fired = 0;

  repeat (BIT_CLOCKS * 4) begin
    @(posedge clk);
    if (rx_valid || rx_error) fired = 1;
  end

  if (fired) begin
    $display("FAIL: verify_idle_line: spurious valid/error on idle line");
    test_failures++;
  end
  test_count++;
endtask

task automatic verify_single_byte;
  logic [7:0] expected  = 8'hA5;
  logic       got_valid = 0;
  // Generous timeout: full frame (10 bit periods) + sync latency + margin
  localparam integer TIMEOUT = BIT_CLOCKS * 12;

  drive_byte(expected);

  // rx_valid pulses for exactly one cycle — capture it on any iteration.
  // Loop runs full duration; no break needed.
  repeat (TIMEOUT) begin
    @(posedge clk);
    if (rx_valid && !got_valid) got_valid = 1;
  end

  if (!got_valid) begin
    $display("FAIL: verify_single_byte: rx_valid never fired");
    test_failures++;
  end else if (rx_data !== expected) begin
    $display("FAIL: verify_single_byte: expected %0x got %0x", expected, rx_data);
    test_failures++;
  end else if (rx_error) begin
    $display("FAIL: verify_single_byte: unexpected rx_error");
    test_failures++;
  end

  test_count++;
endtask

task automatic verify_multiple_bytes;
  logic [7:0] tx_byte;
  logic [7:0] rx_captured;
  logic       got_valid;
  logic       got_error;
  // Timeout covers one full frame with margin
  localparam integer TIMEOUT = BIT_CLOCKS * 12;

  for (int i = 0; i < 32; i++) begin
    tx_byte     = $urandom();
    got_valid   = 0;
    got_error   = 0;
    rx_captured = 0;

    drive_byte(tx_byte);

    repeat (TIMEOUT) begin
      @(posedge clk);
      if (rx_valid && !got_valid) begin
        got_valid   = 1;
        rx_captured = rx_data;  // latch data on the pulse cycle
      end
      if (rx_error) got_error = 1;
    end

    if (!got_valid) begin
      $display("FAIL: verify_multiple_bytes[%0d]: rx_valid never fired", i);
      test_failures++;
    end else if (rx_captured !== tx_byte) begin
      $display("FAIL: verify_multiple_bytes[%0d]: expected %0x got %0x",
               i, tx_byte, rx_captured);
      test_failures++;
    end else if (got_error) begin
      $display("FAIL: verify_multiple_bytes[%0d]: unexpected rx_error", i);
      test_failures++;
    end
  end

  test_count++;
endtask

task automatic verify_framing_error;
  logic got_error = 0;
  logic got_valid = 0;
  localparam integer TIMEOUT = BIT_CLOCKS * 16;

  drive_framing_error(8'hFF);

  repeat (TIMEOUT) begin
    @(posedge clk);
    if (rx_error) got_error = 1;
    if (rx_valid) got_valid = 1;
  end

  if (!got_error) begin
    $display("FAIL: verify_framing_error: rx_error never fired");
    test_failures++;
  end
  if (got_valid) begin
    $display("FAIL: verify_framing_error: rx_valid fired on framing error");
    test_failures++;
  end

  // Allow line to return to idle before next test
  repeat (BIT_CLOCKS * 2) @(posedge clk);

  test_count++;
endtask

task automatic verify_false_start;
  // A start-bit candidate that goes high before midpoint — DUT should reject it
  // and not fire rx_valid or rx_error.
  logic fired = 0;

  @(posedge clk); #1;
  rx_i = 1'b0;
  // Hold low for only a quarter of a bit period — not long enough for midpoint
  repeat (BIT_CLOCKS / 4) @(posedge clk);

  rx_i = 1'b1;
  // Observe for several bit periods
  repeat (BIT_CLOCKS * 3) begin
    @(posedge clk);
    if (rx_valid || rx_error) fired = 1;
  end

  if (fired) begin
    $display("FAIL: verify_false_start: valid/error fired on glitch");
    test_failures++;
  end

  test_count++;
endtask

task automatic diagnose_rx;
  // Drive one start bit and watch raw state signals for 20 clocks
  $display("DIAG: driving start bit, monitoring for 20 clocks");
  @(posedge clk); #1;
  rx_i = 1'b0;

  repeat (20) begin
    @(posedge clk); #1;
    $display("DIAG: t=%0t rx_i=%b rx_s0=%b rx_s1=%b state=%0d sample_div=%0d sample_inc=%b sample_cnt=%0d",
             $time, rx_i, uut.rx_s0, uut.rx_s1, uut.state, uut.sample_div, uut.sample_inc, uut.sample_cnt);
  end

  rx_i = 1'b1;
  repeat (5) @(posedge clk);
endtask

//------------------------------------------------------------------------------
// Test executor
//------------------------------------------------------------------------------
initial begin
  $dumpfile("serial_rx_tb.fst");
  $dumpvars(0, serial_rx_tb);
  $display("TESTBENCH: serial_rx_tb");

  wait (rst_n);
  repeat (4) @(posedge clk);
  #1;

  verify_reset_state();
  verify_idle_line();
  verify_false_start();
   diagnose_rx();        
  verify_single_byte();
  verify_multiple_bytes();
  verify_multiple_bytes();
  verify_framing_error();
  verify_single_byte();    // confirm recovery after framing error

  repeat (5) @(posedge clk);

  if (test_failures == 0) $display("PASS: All %0d tests passed.", test_count);
  else $fatal(1, "FAIL: serial_rx_tb. %0d test(s) failed.", test_failures);

  $finish;
end

endmodule