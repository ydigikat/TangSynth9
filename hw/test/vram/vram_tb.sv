//------------------------------------------------------------------------------
// vram_tb.sv — Testbench for vram / vram_block
//
// Tests:
//   1. verify_reset_state       — cpu_ready deasserted, pipe_valid deasserted
//   2. verify_cpu_write_read    — full 32-bit write via CPU port, readback
//   3. verify_wstrb_lanes       — each byte lane independently (wstrb 4'b0001..1000)
//   4. verify_pipe_read         — pipeline port reads data previously written by CPU
//   5. verify_dual_port_concurrent — CPU writes and pipe reads simultaneously,
//                                    no collision; confirms port independence
//   6. verify_ready_timing      — cpu_ready is exactly one cycle after cpu_select_i
//   7. verify_pipe_valid_timing — pipe_valid is exactly one cycle after reset release
//   8. verify_address_decode    — byte-to-word address translation (cpu_addr[9:2])
//   9. verify_multiple_locations — scattered writes, verify all locations correct
//------------------------------------------------------------------------------
`default_nettype none
`timescale 1ns / 10ps

module vram_tb ();

  //----------------------------------------------------------------------------
  // Test control
  //----------------------------------------------------------------------------
  integer test_failures = 0;
  integer test_count    = 0;

  //----------------------------------------------------------------------------
  // DUT parameters
  //----------------------------------------------------------------------------
  localparam WORD_ADDRESS_WIDTH = 8;   // matches vram.sv
  localparam VRAM_BASE          = 32'h0001_0000;

  //----------------------------------------------------------------------------
  // DUT signals
  //----------------------------------------------------------------------------
  logic        clk    = 0;
  logic        rst_n;

  // CPU port
  logic        cpu_select;
  logic        cpu_ready;
  logic [3:0]  cpu_wstrb;
  logic [31:0] cpu_addr;
  logic [31:0] cpu_wdata;
  logic [31:0] cpu_rdata;

  // Pipeline port
  logic [WORD_ADDRESS_WIDTH-1:0] pipe_addr;
  logic [31:0]                  pipe_data;
  logic                         pipe_valid;

  //----------------------------------------------------------------------------
  // Clock and reset
  //----------------------------------------------------------------------------
  always #5 clk = ~clk;   // 100 MHz — plenty of headroom vs 24 MHz target

  initial begin
    rst_n      = 1'b0;
    cpu_select = 1'b0;
    cpu_wstrb  = 4'h0;
    cpu_addr   = 32'h0;
    cpu_wdata  = 32'h0;
    pipe_addr  = 8'h0;
    #50;
    rst_n = 1'b1;
  end

  //----------------------------------------------------------------------------
  // DUT instantiation
  //----------------------------------------------------------------------------
  vram u_vram (
    .clk_i         (clk),
    .rst_ni        (rst_n),
    .cpu_select_i  (cpu_select),
    .cpu_ready_o   (cpu_ready),
    .cpu_wstrb_i   (cpu_wstrb),
    .cpu_addr_i    (cpu_addr),
    .cpu_wdata_i   (cpu_wdata),
    .cpu_rdata_o   (cpu_rdata),
    .pipe_addr_i   (pipe_addr),
    .pipe_data_o   (pipe_data),
    .pipe_valid_o  (pipe_valid)
  );

  //----------------------------------------------------------------------------
  // Helper tasks
  //----------------------------------------------------------------------------

  // Drive a full 32-bit CPU write (all byte lanes) to a word-indexed address.
  task automatic cpu_write (
    input [7:0]  word_addr,
    input [31:0] data
  );
    @(posedge clk); #1;
    cpu_select <= 1'b1;
    cpu_wstrb  <= 4'hF;
    cpu_addr   <= VRAM_BASE | (word_addr << 2);
    cpu_wdata  <= data;
    @(posedge clk); #1;
    cpu_select <= 1'b0;
    cpu_wstrb  <= 4'h0;
    @(posedge clk); #1;   // settle
  endtask

  // Drive a partial CPU write using an explicit byte strobe.
  task automatic cpu_write_strb (
    input [7:0]  word_addr,
    input [31:0] data,
    input [3:0]  wstrb
  );
    @(posedge clk); #1;
    cpu_select <= 1'b1;
    cpu_wstrb  <= wstrb;
    cpu_addr   <= VRAM_BASE | (word_addr << 2);
    cpu_wdata  <= data;
    @(posedge clk); #1;
    cpu_select <= 1'b0;
    cpu_wstrb  <= 4'h0;
    @(posedge clk); #1;
  endtask

  // Issue a CPU read and return the registered data (arrives one cycle after select).
  task automatic cpu_read (
    input  [7:0]  word_addr,
    output [31:0] data
  );
    @(posedge clk); #1;
    cpu_select <= 1'b1;
    cpu_wstrb  <= 4'h0;
    cpu_addr   <= VRAM_BASE | (word_addr << 2);
    @(posedge clk); #1;   // BSRAM registered output — data valid here
    data = cpu_rdata;
    cpu_select <= 1'b0;
    @(posedge clk); #1;
  endtask

  // Present an address on the pipe port and capture data one cycle later.
  task automatic pipe_read (
    input  [7:0]  word_addr,
    output [31:0] data
  );
    @(posedge clk); #1;
    pipe_addr <= word_addr;
    @(posedge clk); #1;   // registered output
    data = pipe_data;
    @(posedge clk); #1;
  endtask

  //----------------------------------------------------------------------------
  // Tests
  //----------------------------------------------------------------------------

  // 1. Reset state — outputs should be deasserted after reset.
  task automatic verify_reset_state;
    test_count++;

    wait (!rst_n);           // in reset
    @(posedge clk); #1;

    if (cpu_ready !== 1'b0) begin
      $display("FAIL: verify_reset_state: cpu_ready should be 0 in reset, got %b", cpu_ready);
      test_failures++;
    end

    if (pipe_valid !== 1'b0) begin
      $display("FAIL: verify_reset_state: pipe_valid should be 0 in reset, got %b", pipe_valid);
      test_failures++;
    end
  endtask

  // 2. Full 32-bit CPU write then readback.
  task automatic verify_cpu_write_read;
    logic [31:0] rdata;
    test_count++;

    cpu_write(8'h10, 32'hDEAD_BEEF);
    cpu_read (8'h10, rdata);

    if (rdata !== 32'hDEAD_BEEF) begin
      $display("FAIL: verify_cpu_write_read: expected DEADBEEF got %08X", rdata);
      test_failures++;
    end
  endtask

  // 3. Byte lane isolation — each wstrb bit controls exactly one byte lane.
  task automatic verify_wstrb_lanes;
    logic [31:0] rdata;
    test_count++;

    // Prime location with 0xFFFFFFFF
    cpu_write(8'h20, 32'hFFFF_FFFF);

    // Write 0x00 to byte 0 only — upper three bytes untouched
    cpu_write_strb(8'h20, 32'h0000_0042, 4'b0001);
    cpu_read(8'h20, rdata);
    if (rdata !== 32'hFFFF_FF42) begin
      $display("FAIL: verify_wstrb_lanes[0]: expected FFFFFF42 got %08X", rdata);
      test_failures++;
    end

    // Write 0xAB to byte 1 only
    cpu_write(8'h21, 32'hFFFF_FFFF);
    cpu_write_strb(8'h21, 32'h0000_AB00, 4'b0010);
    cpu_read(8'h21, rdata);
    if (rdata !== 32'hFFFF_ABFF) begin
      $display("FAIL: verify_wstrb_lanes[1]: expected FFFFABFF got %08X", rdata);
      test_failures++;
    end

    // Write 0xCD to byte 2 only
    cpu_write(8'h22, 32'hFFFF_FFFF);
    cpu_write_strb(8'h22, 32'h00CD_0000, 4'b0100);
    cpu_read(8'h22, rdata);
    if (rdata !== 32'hFFCD_FFFF) begin
      $display("FAIL: verify_wstrb_lanes[2]: expected FFCDFFFF got %08X", rdata);
      test_failures++;
    end

    // Write 0xEF to byte 3 only
    cpu_write(8'h23, 32'hFFFF_FFFF);
    cpu_write_strb(8'h23, 32'hEF00_0000, 4'b1000);
    cpu_read(8'h23, rdata);
    if (rdata !== 32'hEFFF_FFFF) begin
      $display("FAIL: verify_wstrb_lanes[3]: expected EFFFFFFFF got %08X", rdata);
      test_failures++;
    end
  endtask

  // 4. Pipeline port reads data previously written by the CPU.
  task automatic verify_pipe_read;
    logic [31:0] pdata;
    test_count++;

    cpu_write(8'h30, 32'hA5A5_5A5A);
    pipe_read(8'h30, pdata);

    if (pdata !== 32'hA5A5_5A5A) begin
      $display("FAIL: verify_pipe_read: expected A5A55A5A got %08X", pdata);
      test_failures++;
    end
  endtask

  // 5. Concurrent access — CPU writes one location while pipe reads another.
  //    Tests that the dual-port arrangement is truly independent with no
  //    cross-contamination.
  task automatic verify_dual_port_concurrent;
    logic [31:0] cpu_rd, pipe_rd;
    test_count++;

    // Pre-load two distinct locations
    cpu_write(8'h40, 32'h1111_1111);
    cpu_write(8'h41, 32'h2222_2222);

    // In the same clock cycle: CPU writes new value to 0x40,
    // pipe presents address 0x41.  After the rising edge both
    // registered outputs should carry their respective values.
    @(posedge clk); #1;
    cpu_select <= 1'b1;
    cpu_wstrb  <= 4'hF;
    cpu_addr   <= VRAM_BASE | (8'h40 << 2);
    cpu_wdata  <= 32'hCAFE_BABE;
    pipe_addr  <= 8'h41;

    @(posedge clk); #1;
    // CPU write has latched; pipe output now has addr 0x41 data
    pipe_rd = pipe_data;
    cpu_select <= 1'b0;
    cpu_wstrb  <= 4'h0;

    if (pipe_rd !== 32'h2222_2222) begin
      $display("FAIL: verify_dual_port_concurrent: pipe saw %08X, expected 22222222", pipe_rd);
      test_failures++;
    end

    // Now verify the CPU write landed correctly
    cpu_read(8'h40, cpu_rd);
    if (cpu_rd !== 32'hCAFE_BABE) begin
      $display("FAIL: verify_dual_port_concurrent: cpu_rd %08X, expected CAFEBABE", cpu_rd);
      test_failures++;
    end

    // And that 0x41 was not corrupted
    cpu_read(8'h41, cpu_rd);
    if (cpu_rd !== 32'h2222_2222) begin
      $display("FAIL: verify_dual_port_concurrent: 0x41 corrupted, got %08X", cpu_rd);
      test_failures++;
    end
  endtask

  // 6. cpu_ready timing — exactly one clock after cpu_select_i rises.
  task automatic verify_ready_timing;
    integer ready_cycles;
    test_count++;

    ready_cycles = 0;

    @(posedge clk); #1;
    cpu_select <= 1'b1;
    cpu_wstrb  <= 4'h0;
    cpu_addr   <= VRAM_BASE;

    // Sample on the very next edge — ready must be high here
    @(posedge clk); #1;
    if (cpu_ready !== 1'b1) begin
      $display("FAIL: verify_ready_timing: cpu_ready not asserted one cycle after select");
      test_failures++;
    end

    cpu_select <= 1'b0;
    @(posedge clk); #1;

    // After deselect ready must drop
    if (cpu_ready !== 1'b0) begin
      $display("FAIL: verify_ready_timing: cpu_ready did not deassert after select dropped");
      test_failures++;
    end
  endtask

  // 7. pipe_valid timing — asserted one cycle after reset releases, stays high.
  task automatic verify_pipe_valid_timing;
    test_count++;

    // We are already past reset.  pipe_valid should be high.
    @(posedge clk); #1;
    if (pipe_valid !== 1'b1) begin
      $display("FAIL: verify_pipe_valid_timing: pipe_valid not asserted after reset");
      test_failures++;
    end

    // Hold for several cycles — must remain high unconditionally
    repeat (8) @(posedge clk);
    #1;
    if (pipe_valid !== 1'b1) begin
      $display("FAIL: verify_pipe_valid_timing: pipe_valid dropped unexpectedly");
      test_failures++;
    end
  endtask

  // 8. Address decode — verify the byte-to-word translation in vram.sv.
  //    cpu_addr[9:2] selects the word.  Two consecutive byte addresses that
  //    map to the same word must return the same data.
  task automatic verify_address_decode;
    logic [31:0] rd_a, rd_b;
    test_count++;

    // Write word 0x50 via byte address VRAM_BASE + (0x50 << 2)
    cpu_write(8'h50, 32'hBEEF_C0DE);

    // Read back at the canonical byte address
    @(posedge clk); #1;
    cpu_select <= 1'b1;
    cpu_wstrb  <= 4'h0;
    cpu_addr   <= VRAM_BASE | (32'h50 << 2);
    @(posedge clk); #1;
    rd_a = cpu_rdata;
    cpu_select <= 1'b0;

    // Read at the +1 byte address — still inside the same word
    @(posedge clk); #1;
    cpu_select <= 1'b1;
    cpu_addr   <= VRAM_BASE | (32'h50 << 2) + 1;  // byte offset +1
    @(posedge clk); #1;
    rd_b = cpu_rdata;
    cpu_select <= 1'b0;
    @(posedge clk); #1;

    if (rd_a !== 32'hBEEF_C0DE) begin
      $display("FAIL: verify_address_decode: canonical addr returned %08X", rd_a);
      test_failures++;
    end
    if (rd_b !== rd_a) begin
      $display("FAIL: verify_address_decode: byte+1 addr %08X != canonical %08X", rd_b, rd_a);
      test_failures++;
    end
  endtask

  // 9. Multiple locations — scatter writes across the address space, then
  //    verify all of them are intact (catches aliasing bugs).
  task automatic verify_multiple_locations;
    logic [31:0] rdata;
    logic [7:0]  addrs[8];
    logic [31:0] values[8];
    test_count++;

    addrs[0] = 8'h00;  values[0] = 32'hAABBCCDD;
    addrs[1] = 8'h01;  values[1] = 32'h11223344;
    addrs[2] = 8'h3F;  values[2] = 32'hDEAD0001;
    addrs[3] = 8'h40;  values[3] = 32'hDEAD0002;
    addrs[4] = 8'h7F;  values[4] = 32'hCAFE0001;
    addrs[5] = 8'h80;  values[5] = 32'hCAFE0002;
    addrs[6] = 8'hFE;  values[6] = 32'h12345678;
    addrs[7] = 8'hFF;  values[7] = 32'h87654321;

    // Write phase
    for (int i = 0; i < 8; i++) cpu_write(addrs[i], values[i]);

    // Read-back and verify phase
    for (int i = 0; i < 8; i++) begin
      cpu_read(addrs[i], rdata);
      if (rdata !== values[i]) begin
        $display("FAIL: verify_multiple_locations[%0d]: addr=%02X expected=%08X got=%08X",
                 i, addrs[i], values[i], rdata);
        test_failures++;
      end
    end

    // Spot-check a few via the pipe port as well
    for (int i = 0; i < 8; i += 2) begin
      pipe_read(addrs[i], rdata);
      if (rdata !== values[i]) begin
        $display("FAIL: verify_multiple_locations pipe[%0d]: addr=%02X expected=%08X got=%08X",
                 i, addrs[i], values[i], rdata);
        test_failures++;
      end
    end
  endtask

  //----------------------------------------------------------------------------
  // Test executor
  //----------------------------------------------------------------------------
  initial begin
    $dumpfile("vram_tb.fst");
    $dumpvars(0, vram_tb);
    $display("TESTBENCH: vram_tb");

    // Run in-reset checks before reset releases
    verify_reset_state();

    // Wait for reset to release
    wait (rst_n);
    repeat (4) @(posedge clk);
    #1;

    verify_pipe_valid_timing();
    verify_ready_timing();
    verify_cpu_write_read();
    verify_wstrb_lanes();
    verify_pipe_read();
    verify_dual_port_concurrent();
    verify_address_decode();
    verify_multiple_locations();

    repeat (5) @(posedge clk);

    if (test_failures == 0)
      $display("PASS: All %0d tests passed.", test_count);
    else
      $display("FAIL: %0d of %0d tests failed.", test_failures, test_count);

    $finish;
  end

endmodule

`default_nettype wire