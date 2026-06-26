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
logic       rx_i = 1'b1;
logic [7:0] rx_data;
logic       rx_valid;
logic       rx_error;

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
// Persistent monitors
// monitor_clear lets tasks reset the flags cleanly between tests.
// The always block owns all writes to the flag registers so there
// is no procedural/clocked driver conflict.
//------------------------------------------------------------------------------
logic       monitor_clear   = 1'b0;
logic       rx_valid_seen;
logic       rx_error_seen;
logic [7:0] rx_data_latched;

always @(posedge clk) begin
  if (!rst_n || monitor_clear) begin
    rx_valid_seen   <= 1'b0;
    rx_error_seen   <= 1'b0;
    rx_data_latched <= 8'h00;
  end else begin
    if (rx_valid) begin
      rx_valid_seen   <= 1'b1;
      rx_data_latched <= rx_data;
    end
    if (rx_error)
      rx_error_seen <= 1'b1;
  end
end

// Assert monitor_clear for one clock cycle, straddling a negedge so the
// posedge of the same cycle sees it cleanly without racing rx_valid.
task automatic clear_monitors;
  @(negedge clk); #1;
  monitor_clear = 1'b1;
  @(posedge clk); #1;
  monitor_clear = 1'b0;
endtask

//------------------------------------------------------------------------------
// drive_byte — UART frame: START(0) | D0..D7 | STOP(1)
// rx_valid fires at the stop-bit midpoint, still inside this task.
// The persistent monitor catches it; tasks check rx_valid_seen after returning.
//------------------------------------------------------------------------------
task automatic drive_byte(logic [7:0] data);
  @(posedge clk); #1;
  rx_i = 1'b0;
  repeat (BIT_CLOCKS) @(posedge clk);

  for (int i = 0; i < 8; i++) begin
    rx_i = data[i];
    repeat (BIT_CLOCKS) @(posedge clk);
  end

  rx_i = 1'b1;
  repeat (BIT_CLOCKS) @(posedge clk);
endtask

//------------------------------------------------------------------------------
// drive_framing_error — stop bit held low
//------------------------------------------------------------------------------
task automatic drive_framing_error(logic [7:0] data);
  @(posedge clk); #1;
  rx_i = 1'b0;
  repeat (BIT_CLOCKS) @(posedge clk);

  for (int i = 0; i < 8; i++) begin
    rx_i = data[i];
    repeat (BIT_CLOCKS) @(posedge clk);
  end

  rx_i = 1'b0;
  repeat (BIT_CLOCKS) @(posedge clk);

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

task automatic verify_false_start;
  logic fired = 0;

  clear_monitors();

  @(posedge clk); #1;
  rx_i = 1'b0;
  repeat (BIT_CLOCKS / 4) @(posedge clk);

  rx_i = 1'b1;
  repeat (BIT_CLOCKS * 3) begin
    @(posedge clk);
    if (rx_valid || rx_error) fired = 1;
  end

  if (fired) begin
    $display("FAIL: verify_false_start: valid/error fired on glitch");
    test_failures++;
  end

  // Let DUT fully return to Idle before next test
  repeat (BIT_CLOCKS) @(posedge clk);

  test_count++;
endtask

task automatic verify_single_byte;
  logic [7:0] expected = 8'hA5;

  clear_monitors();
  drive_byte(expected);

  if (!rx_valid_seen) begin
    $display("FAIL: verify_single_byte: rx_valid never fired");
    test_failures++;
  end else if (rx_data_latched !== expected) begin
    $display("FAIL: verify_single_byte: expected %0x got %0x",
             expected, rx_data_latched);
    test_failures++;
  end else if (rx_error_seen) begin
    $display("FAIL: verify_single_byte: unexpected rx_error");
    test_failures++;
  end

  test_count++;
endtask

task automatic verify_multiple_bytes;
  logic [7:0] tx_byte;

  for (int i = 0; i < 32; i++) begin
    tx_byte = $urandom();

    clear_monitors();
    drive_byte(tx_byte);

    if (!rx_valid_seen) begin
      $display("FAIL: verify_multiple_bytes[%0d]: rx_valid never fired", i);
      test_failures++;
    end else if (rx_data_latched !== tx_byte) begin
      $display("FAIL: verify_multiple_bytes[%0d]: expected %0x got %0x",
               i, tx_byte, rx_data_latched);
      test_failures++;
    end else if (rx_error_seen) begin
      $display("FAIL: verify_multiple_bytes[%0d]: unexpected rx_error", i);
      test_failures++;
    end
  end

  test_count++;
endtask

task automatic verify_framing_error;
  clear_monitors();
  drive_framing_error(8'hFF);

  if (!rx_error_seen) begin
    $display("FAIL: verify_framing_error: rx_error never fired");
    test_failures++;
  end
  if (rx_valid_seen) begin
    $display("FAIL: verify_framing_error: rx_valid fired on framing error");
    test_failures++;
  end

  repeat (BIT_CLOCKS * 2) @(posedge clk);

  test_count++;
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
  verify_single_byte();
  verify_multiple_bytes();
  verify_multiple_bytes();
  verify_framing_error();
  //verify_single_byte();   

  repeat (5) @(posedge clk);

  if (test_failures == 0) $display("PASS: All %0d tests passed.", test_count);
  else $fatal(1, "FAIL: serial_rx_tb. %0d test(s) failed.", test_failures);

  $finish;
end

endmodule