//------------------------------------------------------------------------------
// Jason Wilden 2025
//------------------------------------------------------------------------------
`default_nettype none 
`timescale 1ns / 10ps

module uart_tx_tb ();

//------------------------------------------------------------------------------
// Test control
//------------------------------------------------------------------------------
integer test_failures = 0;
integer test_count = 0;

//------------------------------------------------------------------------------
// DUT parameters and signals
//------------------------------------------------------------------------------
localparam logic[10:0] DVSR = 16; 
localparam BIT_PERIOD = (DVSR + 1) * 16 * 10;    

logic clk = 0;
logic rst_n;
logic start,done,tx;
logic[7:0] data;

//------------------------------------------------------------------------------
// Clock generation
//------------------------------------------------------------------------------
always #5 clk = ~clk;

//------------------------------------------------------------------------------
// Unit under test
//------------------------------------------------------------------------------
uart_tx uut (
  .clk_i(clk),
  .rst_ni(rst_n),
  .div_i(DVSR),
  .tx_start_i(start),
  .data_i(data),
  .tx_done_o(done),
  .tx_o(tx)
);

//------------------------------------------------------------------------------
// Helper function to verify a UART byte
// UART format: START(0) + 8 DATA bits (LSB first) + STOP(1)
//------------------------------------------------------------------------------
task automatic verify_sent_byte(logic[7:0] expected);
  logic start_bit = 1, stop_bit = 0;
  logic[7:0] rx_byte = 0;

  // Wait for line to go low
  wait(!tx);

  // Start bit
  start_bit = tx;  
  repeat (BIT_PERIOD/10) @(posedge clk);
  
  for(integer i = 0; i <= 7; i++) begin
    rx_byte[i] = tx;
    repeat (BIT_PERIOD/10) @(posedge clk);
  end

  // Stop bit
  stop_bit = tx;
  repeat (BIT_PERIOD/10) @(posedge clk);

  // Wait for byte to complete
  wait(done);

  // $display("Bit count: %0d",uut.bit_cnt);
  // $display("Start bit: %0b", start_bit);
  // $display("RX Byte: %0x",rx_byte);
  // $display("Stop bit: %0b", stop_bit);

  if(rx_byte != expected) begin
    $display("FAIL: expected %0x but got %0x",expected,rx_byte);
  end

endtask

//------------------------------------------------------------------------------
// Tests
//------------------------------------------------------------------------------
task automatic verify_reset_state;
  @(posedge clk);
  #1;
  
  if (done != 1'b1 || tx != 1'b1) begin
    $display("FAIL: verify_reset_state. Invalid starting state");
    test_failures++;
  end
  test_count++;
endtask

task automatic verify_single_byte;
  data = 'hA5;

  @(posedge clk);
  #1;

  // Start data serialisation
  start = 1'b1;
  verify_sent_byte('hA5);
  start = 1'b0;


  @(posedge clk);
  #1;
  test_count++;

endtask

task automatic verify_multiple_bytes;

  logic[7:0] tx_byte;

  for(int i = 0; i < 32; i++) begin
    tx_byte = $urandom();
    //$display("Sending %0h",tx_byte);
    data = tx_byte;

    @(posedge clk);
    #1;

    // Start data serialisation 
    start = 1'b1;
    verify_sent_byte(tx_byte);
    start = 1'b0;
  end
  test_count++;
endtask


//------------------------------------------------------------------------------
// Test executor
//------------------------------------------------------------------------------
initial begin
  $dumpfile("uart_tx.fst");
  $dumpvars(0, uart_tx_tb);
  $display("TESTBENCH: uart_tx_tb");

  // Initialize
  rst_n = 0;
  #50;
  rst_n = 1;
  
  // Wait for stable state
  repeat (2) @(posedge clk);
  #1;

  start = 1'b0;

  verify_reset_state();
  verify_single_byte();
  verify_multiple_bytes();
  verify_multiple_bytes();
  
  repeat (5) @(posedge clk);

  if (test_failures == 0) $display("PASS: All %0d tests passed.",test_count);
  else $fatal(1, "FAIL: uart_tx_tb. %0d test(s) failed.", test_failures);

  $finish;
end

endmodule
