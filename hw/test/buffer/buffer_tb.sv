//------------------------------------------------------------------------------
// Jason Wilden 2025
//------------------------------------------------------------------------------
// 1. Verify that the buffer is correct after reset
// 2. Verify a single write followed by read operation
// 3. Verify the bufffer is filled by the correct number of writes
// 4. Verify bufffer draining
// 5. Verify write to full buffer is handled correctly
// 6. Verify read when empty
// 7. Verify wrap around behaviour
//------------------------------------------------------------------------------

`default_nettype none 
`timescale 1ns / 10ps

module buffer_tb ();

  //------------------------------------------------------------------------------
  // Test control counts
  //------------------------------------------------------------------------------
  integer test_failures = 0;
  integer test_count = 0;

  //------------------------------------------------------------------------------
  // DUT parameters and signals
  //------------------------------------------------------------------------------
  localparam BUF_SIZE = 8;
  localparam DATA_WIDTH = 16;

  logic clk = 0;
  logic rst_n;
  logic write_en;
  logic [DATA_WIDTH-1:0] write_data;
  logic read_en;
  logic [DATA_WIDTH-1:0] read_data;
  logic empty;
  logic full;

  //------------------------------------------------------------------------------
  // Clocks and resets
  //------------------------------------------------------------------------------
  always #5 clk = ~clk;

  initial begin
    rst_n = 0;
    write_en = 0;
    read_en = 0;
    write_data = 0;
    #50;
    rst_n = 1;
  end

  //------------------------------------------------------------------------------
  // Unit under test instance
  //------------------------------------------------------------------------------
  buffer #(
      .BUF_SIZE(BUF_SIZE),
      .DATA_WIDTH(DATA_WIDTH)
  ) uut (
      .clk_i(clk),
      .rst_ni(rst_n),
      .wr_i(write_en),      
      .rd_i(read_en),
      .wdata_i(write_data),
      .rdata_o(read_data),
      .empty_o(empty),
      .full_o(full)
  );

  //------------------------------------------------------------------------------
  // Tests
  //------------------------------------------------------------------------------

  // 1. Verify that the buffer is correct after reset
  task automatic verify_reset_state;
    test_count++;    

    @(posedge clk);
    #1;

    if (empty !== 1'b1) begin
      $display("FAIL: Buffer should be empty after reset");
      test_failures++;      
    end

    if (full !== 1'b0) begin
      $display("FAIL: Buffer should not be full after reset");
      test_failures++;      
    end    
    
  endtask

  // 2. Verify a single write followed by read operation
  task automatic verify_single_write_read;
    logic [DATA_WIDTH-1:0] test_value;

    test_count++;    
    
    test_value = 16'hABCD;

    @(posedge clk);
    #1;
    write_en   = 1;
    write_data = test_value;

    @(posedge clk);
    #1;
    write_en = 0;

    if (empty !== 1'b0) begin
      $display("FAIL: Buffer should not be empty after write");
      test_failures++;      
    end

    // Wait a cycle for write to complete
    @(posedge clk);
    #1;

    // Assert read_en and sample data in same cycle
    read_en = 1;
    if (read_data !== test_value) begin
      $display("FAIL: Read data mismatch. Expected: %h, Got: %h", test_value, read_data);
      test_failures++;      
    end

    @(posedge clk);
    #1;
    read_en = 0;

    if (empty !== 1'b1) begin
      $display("FAIL: Buffer should be empty after reading last item");
      test_failures++;      
    end
  endtask

  // 3. Verify the bufffer is filled by the correct number of writes
  task automatic verify_fill_buffer;
    integer i;

    test_count++;    

    for (i = 0; i < BUF_SIZE; i++) begin
      @(posedge clk);
      #1;
      write_en   = 1;
      write_data = i;
    end

    @(posedge clk);
    #1;
    write_en = 0;

    if (full !== 1'b1) begin
      $display("FAIL: Buffer should be full after %0d writes", BUF_SIZE);
      test_failures++;      
    end

    if (empty !== 1'b0) begin
      $display("FAIL: Buffer should not be empty when full");
      test_failures++;      
    end
  endtask


  // 4. Verify bufffer draining
  task automatic verify_drain_buffer;
    integer i;

    test_count++;    
    

    for (i = 0; i < BUF_SIZE; i++) begin
      @(posedge clk);
      #1;

      // Assert read and sample data in same cycle
      read_en = 1;
      if (read_data !== i) begin
        $display("FAIL: Read data mismatch at position %0d. Expected: %h, Got: %h", i, i,
                 read_data);
        test_failures++;
        
      end

      @(posedge clk);
      #1;
      read_en = 0;
    end

    @(posedge clk);
    #1;

    if (empty !== 1'b1) begin
      $display("FAIL: Buffer should be empty after draining");
      test_failures++;
    end

    if (full !== 1'b0) begin
      $display("FAIL: Buffer should not be full after draining");
      test_failures++;      
    end


  endtask


  // 5. Verify write to full buffer is handled correctly
  task automatic verify_write_when_full;
    integer i;
    
    test_count++;    

    // Fill buffer to capacity
    for (i = 0; i < BUF_SIZE; i++) begin
      @(posedge clk);
      #1;
      write_en   = 1;
      write_data = i + 100;
    end

    // Attempt additional write
    @(posedge clk);
    #1;
    write_data = 16'hDEAD;

    @(posedge clk);
    #1;
    write_en = 0;

    // Assert read and sample data in same cycle
    @(posedge clk);
    #1;
    read_en = 1;

    if (read_data !== 100) begin
      $display("FAIL: Write when full corrupted data. Expected: %h, Got: %h", 100, read_data);
      test_failures++;
    end

    @(posedge clk);
    #1;
    read_en = 0;

    // Drain remaining
    for (i = 1; i < BUF_SIZE; i++) begin
      @(posedge clk);
      #1;
      read_en = 1;
      @(posedge clk);
      #1;
      read_en = 0;
    end    
  endtask

  // 6. Verify read when empty is benign
  task automatic verify_read_when_empty;    

    test_count++;     

    @(posedge clk);
    #1;
    read_en = 1;

    @(posedge clk);
    #1;
    read_en = 0;

    if (empty !== 1'b1) begin
      $display("FAIL: Buffer state corrupted by read when empty");
      test_failures++;      
    end
    
  endtask

  // 7. Verify wrap around behaviour
  task automatic verify_wraparound;
    integer i;

    test_count++;    

    // Write and read to force wraparound
    for (i = 0; i < BUF_SIZE * 2; i++) begin
      @(posedge clk);
      #1;
      write_en   = 1;
      write_data = i;

      @(posedge clk);
      #1;
      write_en = 0;

      // Wait a cycle for write to complete
      @(posedge clk);
      #1;

      // Assert read and sample data in same cycle
      read_en = 1;
      if (read_data !== i) begin
        $display("FAIL: Wraparound data mismatch at iteration %0d. Expected: %h, Got: %h", i, i,
                 read_data);
        test_failures++;        
      end

      @(posedge clk);
      #1;
      read_en = 0;
    end    
  endtask

  //------------------------------------------------------------------------------
  // Test executor
  //------------------------------------------------------------------------------
  initial begin
    $dumpfile("buffer.fst");
    $dumpvars(0, buffer_tb);
    $display("TESTBENCH: buffer_tb");

    // Wait for a stable uut
    wait (rst_n);
    repeat (2) @(posedge clk);

    verify_reset_state();
    verify_single_write_read();
    verify_fill_buffer();
    verify_drain_buffer();
    verify_write_when_full();
    verify_read_when_empty();
    verify_wraparound();

    repeat (5) @(posedge clk);

    if (test_failures == 0) $display("PASS: All %0d tests passed.",test_count);
    else $fatal(1, "FAIL: ring_buffer_tb. %0d test(s) failed.", test_failures);

    $finish;
  end

endmodule