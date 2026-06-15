//------------------------------------------------------------------------------
// (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "../defs.svh"

module buffer #(
    parameter unsigned BUF_SIZE=16,
    parameter unsigned DATA_WIDTH=8,
    parameter unsigned BUF_ADDR_SIZE=$clog2(BUF_SIZE)
) (
    input `VAR logic                  clk_i,
    input `VAR logic                  rst_ni,

    input `VAR logic                  wr_i, rd_i,
    input `VAR logic [DATA_WIDTH-1:0] wdata_i,
    output     logic [DATA_WIDTH-1:0] rdata_o,

    output     logic                  empty_o,
    output     logic                  full_o
);


  typedef logic [BUF_ADDR_SIZE-1:0] buf_ptr_t;

  logic [DATA_WIDTH-1:0] buffer[BUF_SIZE];
  buf_ptr_t head, head_d;
  buf_ptr_t tail, tail_d;
  logic full, full_d;

  //-----------------------------------------------------------------------------
  // State registers
  //-----------------------------------------------------------------------------
  always_ff @(posedge clk_i) begin
    if (!rst_ni) begin
      head <= 0;
      tail <= 0;
      full <= 0;
    end else begin      
      head <= head_d;
      tail <= tail_d;
      full <= full_d;
    end
  end

  //-----------------------------------------------------------------------------
  // Buffer write
  //-----------------------------------------------------------------------------
  always_ff @(posedge clk_i) begin
    if (wr_i && !full_o) begin
      buffer[head] <= wdata_i;
    end
  end

  //-----------------------------------------------------------------------------
  // Next state logic
  //-----------------------------------------------------------------------------
  always_comb begin
    head_d = head;
    tail_d = tail;
    full_d = full;

    if (wr_i && !full_o) begin
      head_d = head + 1'd1;  
      if (head_d == tail && !rd_i) begin
        full_d = 1;
      end
    end

    if (rd_i && !empty_o) begin
      tail_d = tail + 1'd1;  
      full_d = 0;
    end
  end

  //-----------------------------------------------------------------------------
  // Output logic
  //-----------------------------------------------------------------------------
  assign rdata_o = buffer[tail];
  assign empty_o = (head == tail) && !full;
  assign full_o = full;

endmodule

`default_nettype wire