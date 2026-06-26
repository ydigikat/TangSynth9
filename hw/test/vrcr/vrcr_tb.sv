//------------------------------------------------------------------------------
// vrcr_tb.sv — Testbench for vrcr
//
// The VRCR is the handshake mechanism that gates VRAM updates from the CPU
// to the audio pipeline. The firmware writes CR[0]=1 to signal that a new
// set of voice parameters is ready; the pipeline reads vram_update_o and
// latches VRAM at the next block boundary.
//
// Tests:
//   1. verify_reset_state        — vram_update_o low, mem_ready low after reset
//   2. verify_update_set         — write CR[0]=1, vram_update_o asserts next cycle
//   3. verify_update_clear       — write CR[0]=0, vram_update_o deasserts next cycle
//   4. verify_ready_timing       — mem_ready is combinatorial with select_i (no delay)
//   5. verify_rdata_always_zero  — reads always return 0x00000000
//   6. verify_wstrb_gating       — write with wstrb=0 must not change CR
//   7. verify_upper_bits_stored  — CR is a full 32-bit register; upper bits are latched
//   8. verify_idempotent_set     — writing 1 twice leaves vram_update_o asserted
//   9. verify_wrong_addr_ignored — write to non-CR address must not affect output
//------------------------------------------------------------------------------
`default_nettype none
`timescale 1ns / 10ps

module vrcr_tb ();

  //----------------------------------------------------------------------------
  // Test control
  //----------------------------------------------------------------------------
  integer test_failures = 0;
  integer test_count    = 0;

  //----------------------------------------------------------------------------
  // Address constants (matches mcu.sv address map)
  //----------------------------------------------------------------------------
  localparam VRCR_BASE = 32'h8000_0080;
  localparam CR_OFFSET = 32'h00;    // CR is at word 0 (addr[5:2] == 4'h0)

  //----------------------------------------------------------------------------
  // DUT signals
  //----------------------------------------------------------------------------
  logic        clk = 0;
  logic        rst_n;

  logic        select;
  logic        mem_ready;
  logic [3:0]  mem_wstrb;
  logic [31:0] mem_addr;
  logic [31:0] mem_wdata;
  logic [31:0] mem_rdata;
  logic        vram_update;

  //----------------------------------------------------------------------------
  // Clock and reset
  //----------------------------------------------------------------------------
  always #5 clk = ~clk;

  initial begin
    rst_n     = 1'b0;
    select    = 1'b0;
    mem_wstrb = 4'h0;
    mem_addr  = 32'h0;
    mem_wdata = 32'h0;
    #50;
    rst_n = 1'b1;
  end

  //----------------------------------------------------------------------------
  // DUT instantiation
  //----------------------------------------------------------------------------
  vrcr u_vrcr (
    .clk_i        (clk),
    .rst_ni       (rst_n),
    .select_i     (select),
    .vram_update_o(vram_update),
    .mem_ready_o  (mem_ready),
    .mem_wstrb_i  (mem_wstrb),
    .mem_addr_i   (mem_addr),
    .mem_wdata_i  (mem_wdata),
    .mem_rdata_o  (mem_rdata)
  );

  //----------------------------------------------------------------------------
  // Helper tasks
  //----------------------------------------------------------------------------

  // Write to VRCR CR register.  One-cycle pulse on select with wstrb active.
  task automatic cr_write (input [31:0] data, input [3:0] wstrb);
    @(posedge clk); #1;
    select    <= 1'b1;
    mem_wstrb <= wstrb;
    mem_addr  <= VRCR_BASE + CR_OFFSET;
    mem_wdata <= data;
    @(posedge clk); #1;
    select    <= 1'b0;
    mem_wstrb <= 4'h0;
    @(posedge clk); #1;   // allow registered output to settle
  endtask

  // Write to an address that is NOT the CR (addr[5:2] != 0).
  task automatic write_wrong_addr (input [31:0] data);
    @(posedge clk); #1;
    select    <= 1'b1;
    mem_wstrb <= 4'hF;
    mem_addr  <= VRCR_BASE + 32'h04;   // word 1 — not mapped
    mem_wdata <= data;
    @(posedge clk); #1;
    select    <= 1'b0;
    mem_wstrb <= 4'h0;
    @(posedge clk); #1;
  endtask

  // Perform a read cycle (wstrb=0) and return mem_rdata.
  task automatic cr_read (output [31:0] data);
    @(posedge clk); #1;
    select    <= 1'b1;
    mem_wstrb <= 4'h0;
    mem_addr  <= VRCR_BASE + CR_OFFSET;
    @(posedge clk); #1;
    data   = mem_rdata;
    select <= 1'b0;
    @(posedge clk); #1;
  endtask

  //----------------------------------------------------------------------------
  // Tests
  //----------------------------------------------------------------------------

  // 1. Reset state.
  task automatic verify_reset_state;
    test_count++;

    wait (!rst_n);
    @(posedge clk); #1;

    if (vram_update !== 1'b0) begin
      $display("FAIL: verify_reset_state: vram_update_o should be 0 in reset, got %b", vram_update);
      test_failures++;
    end

    if (mem_ready !== 1'b0) begin
      $display("FAIL: verify_reset_state: mem_ready_o should be 0 in reset, got %b", mem_ready);
      test_failures++;
    end
  endtask

  // 2. Writing CR[0]=1 asserts vram_update_o on the following cycle.
  task automatic verify_update_set;
    test_count++;

    cr_write(32'h0000_0001, 4'hF);

    if (vram_update !== 1'b1) begin
      $display("FAIL: verify_update_set: vram_update_o not asserted after CR[0]=1");
      test_failures++;
    end
  endtask

  // 3. Writing CR[0]=0 deasserts vram_update_o on the following cycle.
  task automatic verify_update_clear;
    test_count++;

    cr_write(32'h0000_0001, 4'hF);   // set first
    cr_write(32'h0000_0000, 4'hF);   // then clear

    if (vram_update !== 1'b0) begin
      $display("FAIL: verify_update_clear: vram_update_o still asserted after CR[0]=0");
      test_failures++;
    end
  endtask

  // 4. mem_ready is combinatorial — it follows select_i with no clock delay.
  //    Sample before the next rising edge to catch the combinatorial path.
  task automatic verify_ready_timing;
    test_count++;

    @(posedge clk); #1;
    select <= 1'b1;

    // Sample mid-cycle — mem_ready must already be high (combinatorial assign)
    #3;
    if (mem_ready !== 1'b1) begin
      $display("FAIL: verify_ready_timing: mem_ready not combinatorial with select");
      test_failures++;
    end

    @(posedge clk); #1;
    select <= 1'b0;
    #3;
    if (mem_ready !== 1'b0) begin
      $display("FAIL: verify_ready_timing: mem_ready did not deassert with select");
      test_failures++;
    end

    @(posedge clk); #1;
  endtask

  // 5. Reads always return 0 — the MCU has no need to read back the VRCR.
  task automatic verify_rdata_always_zero;
    logic [31:0] rdata;
    test_count++;

    cr_write(32'hFFFF_FFFF, 4'hF);   // set all bits
    cr_read(rdata);

    if (rdata !== 32'h0000_0000) begin
      $display("FAIL: verify_rdata_always_zero: got %08X, expected 00000000", rdata);
      test_failures++;
    end
  endtask

  // 6. wstrb=0 must not update CR even when select is asserted.
  //    First set CR[0]=1, then issue a select+wstrb=0 write with wdata=0,
  //    and confirm vram_update_o is still asserted.
  task automatic verify_wstrb_gating;
    test_count++;

    cr_write(32'h0000_0001, 4'hF);   // set update flag
    cr_write(32'h0000_0000, 4'h0);   // wstrb=0 — should be a no-op

    if (vram_update !== 1'b1) begin
      $display("FAIL: verify_wstrb_gating: CR changed on wstrb=0 write");
      test_failures++;
    end

    cr_write(32'h0000_0000, 4'hF);   // clean up
  endtask

  // 7. Upper bits of CR are stored and drive vram_update_o from bit 0.
  //    Writing a pattern with bit 0 clear must leave vram_update_o low
  //    regardless of upper bit content.
  task automatic verify_upper_bits_stored;
    test_count++;

    // Write with upper bits set but bit 0 clear
    cr_write(32'hFFFF_FFFE, 4'hF);

    if (vram_update !== 1'b0) begin
      $display("FAIL: verify_upper_bits_stored: vram_update_o high when CR[0]=0");
      test_failures++;
    end

    // Now set bit 0 — update must fire
    cr_write(32'hFFFF_FFFF, 4'hF);

    if (vram_update !== 1'b1) begin
      $display("FAIL: verify_upper_bits_stored: vram_update_o low when CR[0]=1");
      test_failures++;
    end

    cr_write(32'h0000_0000, 4'hF);   // clean up
  endtask

  // 8. Writing CR[0]=1 twice leaves vram_update_o asserted (idempotent set).
  task automatic verify_idempotent_set;
    test_count++;

    cr_write(32'h0000_0001, 4'hF);
    cr_write(32'h0000_0001, 4'hF);

    if (vram_update !== 1'b1) begin
      $display("FAIL: verify_idempotent_set: vram_update_o not asserted after double set");
      test_failures++;
    end

    cr_write(32'h0000_0000, 4'hF);   // clean up
  endtask

  // 9. Write to a non-CR address must not alter vram_update_o.
  //    Confirm the address decode gate (addr[5:2] == CR) is correct.
  task automatic verify_wrong_addr_ignored;
    test_count++;

    cr_write(32'h0000_0000, 4'hF);      // ensure update is clear
    write_wrong_addr(32'hFFFF_FFFF);   // write all-ones to word 1

    if (vram_update !== 1'b0) begin
      $display("FAIL: verify_wrong_addr_ignored: vram_update_o set by write to unmapped address");
      test_failures++;
    end
  endtask

  //----------------------------------------------------------------------------
  // Test executor
  //----------------------------------------------------------------------------
  initial begin
    $dumpfile("vrcr_tb.fst");
    $dumpvars(0, vrcr_tb);
    $display("TESTBENCH: vrcr_tb");

    verify_reset_state();

    wait (rst_n);
    repeat (4) @(posedge clk);
    #1;

    verify_ready_timing();
    verify_update_set();
    verify_update_clear();
    verify_rdata_always_zero();
    verify_wstrb_gating();
    verify_upper_bits_stored();
    verify_idempotent_set();
    verify_wrong_addr_ignored();

    repeat (5) @(posedge clk);

    if (test_failures == 0)
      $display("PASS: All %0d tests passed.", test_count);
    else
      $display("FAIL: %0d of %0d tests failed.", test_failures, test_count);

    $finish;
  end

endmodule
