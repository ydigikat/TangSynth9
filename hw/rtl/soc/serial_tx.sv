//------------------------------------------------------------------------------
// (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "../defs.svh"

module serial_tx (
  input  `VAR logic        clk_i,
  input  `VAR logic        rst_ni,
  input  `VAR logic [10:0] div_i,
  input  `VAR logic [7:0]  data_i,
  input  `VAR logic        tx_start_i,
  output      logic        tx_done_o,
  output      logic        tx_o
);

//------------------------------------------------------------------------------
// State machine encoding
//------------------------------------------------------------------------------
typedef enum logic [2:0] 
{
  Idle,
  Start,
  Data,
  Stop
} uart_state_t;

uart_state_t state, state_d;

//------------------------------------------------------------------------------
// Registers
//------------------------------------------------------------------------------
logic [10:0] sample_div, sample_div_d;
logic [3:0]  sample_cnt, sample_cnt_d;
logic [2:0]  bit_cnt, bit_cnt_d;
logic [7:0]  data, data_d;

// Output register
logic tx, tx_d;

always_ff @(posedge clk_i) begin
  if (!rst_ni) begin    
    state      <= Idle;
    bit_cnt    <= '0;
    sample_cnt <= '0;
    data       <= '0;
    sample_div <= '0;
    tx         <= 1'b1;
  end else begin
    state      <= state_d;
    bit_cnt    <= bit_cnt_d;
    sample_cnt <= sample_cnt_d;
    data       <= data_d;
    sample_div <= sample_div_d;
    tx         <= tx_d;
  end
end

//------------------------------------------------------------------------------
// Sample rate divider
//------------------------------------------------------------------------------
logic sample_inc;
assign sample_inc   = (sample_div == div_i);
assign sample_div_d = sample_inc ? 11'd0 : (sample_div + 11'd1);


//------------------------------------------------------------------------------
// Next-state / Output logic
//------------------------------------------------------------------------------
always_comb begin
  // Default hold
  state_d      = state;
  bit_cnt_d    = bit_cnt;
  sample_cnt_d = sample_cnt;
  data_d       = data;
  tx_d         = tx;

  unique case (state)
    // Idle — wait for tx_start_i, keep line high
    Idle: begin
      tx_d = 1'b1;
      if (tx_start_i) begin
        state_d      = Start;
        sample_cnt_d = 4'd0;
        data_d   = data_i;
      end
    end
    // Start bit — hold low for 16 samples
    Start: begin
      tx_d = 1'b0;
      if (sample_inc) begin
        if (sample_cnt == 4'd15) begin
          state_d      = Data;
          sample_cnt_d = 4'd0;
          bit_cnt_d    = 3'd0;
        end else begin
          sample_cnt_d = sample_cnt + 1'd1;
        end
      end
    end
    // Data bits — shift out LSB-first, 16 samples per bit
    Data: begin
      tx_d = data[0];
      if (sample_inc) begin
        if (sample_cnt == 4'd15) begin
          sample_cnt_d = 4'd0;
          data_d   = {1'b0, data[7:1]};        
          if (bit_cnt == 3'd7) state_d = Stop;
          else bit_cnt_d = bit_cnt + 1'd1;
        end else begin
          sample_cnt_d = sample_cnt + 1'd1;
        end
      end
    end
    // Stop bit — high for 16 samples
    Stop: begin
      tx_d = 1'b1;
      if (sample_inc) begin
        if (sample_cnt == 4'd15) state_d = Idle;
        else sample_cnt_d = sample_cnt + 1'd1;        
      end
    end
    default: begin
    end
  endcase
end

//------------------------------------------------------------------------------
// Outputs
//------------------------------------------------------------------------------
assign tx_o      = tx;
assign tx_done_o = (state == Idle);

endmodule

`default_nettype wire

