//------------------------------------------------------------------------------
// (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "../defs.svh"

module serial_rx (
  input  `VAR logic        clk_i,
  input  `VAR logic        rst_ni,
  input  `VAR logic [10:0] div_i,
  input  `VAR logic        rx_i,
  output      logic [7:0]  rx_data_o,
  output      logic        rx_valid_o,
  output      logic        rx_error_o
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
// Input synchroniser  (2-FF metastability chain)
//------------------------------------------------------------------------------
logic rx_s0, rx_s1;

always_ff @(posedge clk_i) begin
  if (!rst_ni) begin
    rx_s0 <= 1'b1;
    rx_s1 <= 1'b1;
  end else begin
    rx_s0 <= rx_i;
    rx_s1 <= rx_s0;
  end
end

//------------------------------------------------------------------------------
// Registers
//------------------------------------------------------------------------------
logic [10:0] sample_div, sample_div_d;
logic [3:0]  sample_cnt, sample_cnt_d;
logic [2:0]  bit_cnt,    bit_cnt_d;
logic [7:0]  data,       data_d;

// Output registers
logic        rx_valid,   rx_valid_d;
logic        rx_error,   rx_error_d;

always_ff @(posedge clk_i) begin
  if (!rst_ni) begin
    state      <= Idle;
    bit_cnt    <= '0;
    sample_cnt <= '0;
    data       <= '0;
    sample_div <= '0;
    rx_valid   <= 1'b0;
    rx_error   <= 1'b0;
  end else begin
    state      <= state_d;
    bit_cnt    <= bit_cnt_d;
    sample_cnt <= sample_cnt_d;
    data       <= data_d;
    sample_div <= sample_div_d;
    rx_valid   <= rx_valid_d;
    rx_error   <= rx_error_d;
  end
end

//------------------------------------------------------------------------------
// Sample rate divider
// Runs continuously so the baud clock is always available.
// Resets on start-bit detection to align sample windows to the incoming byte.
//------------------------------------------------------------------------------
logic sample_inc;
assign sample_inc   = (sample_div == div_i);
assign sample_div_d = (state == Idle) ? '0 :
                       sample_inc      ? '0 : (sample_div + 11'd1);

//------------------------------------------------------------------------------
// Next-state / output logic
//------------------------------------------------------------------------------
always_comb begin
  // Default hold
  state_d      = state;
  bit_cnt_d    = bit_cnt;
  sample_cnt_d = sample_cnt;
  data_d       = data;
  rx_valid_d   = 1'b0;   // Pulse — clear every cycle
  rx_error_d   = rx_error;

  unique case (state)

    // Idle — line is high. A falling edge is a start-bit candidate.
    Idle: begin
      rx_error_d   = 1'b0;
      if (!rx_s1) begin
        state_d      = Start;
        sample_cnt_d = 4'd0;
      end
    end

    // Start bit — wait for the midpoint (sample 8) then verify line is still low.
    // Resampling at centre avoids committing to a glitch.
    Start: begin
      if (sample_inc) begin
        if (sample_cnt == 4'd7) begin
          if (!rx_s1) begin
            // Valid start bit confirmed — move to data, reset for bit windows
            state_d      = Data;
            sample_cnt_d = 4'd0;
            bit_cnt_d    = 3'd0;
          end else begin
            // Glitch — false start, back to idle
            state_d = Idle;
          end
        end else begin
          sample_cnt_d = sample_cnt + 4'd1;
        end
      end
    end

    // Data bits — sample at midpoint of each bit window (sample 8), shift in LSB first.
    Data: begin
      if (sample_inc) begin
        if (sample_cnt == 4'd15) begin
          sample_cnt_d = 4'd0;
          data_d       = {rx_s1, data[7:1]};   // Shift right, MSB from line
          if (bit_cnt == 3'd7) begin
            state_d = Stop;
          end else begin
            bit_cnt_d = bit_cnt + 3'd1;
          end
        end else begin
          sample_cnt_d = sample_cnt + 4'd1;
        end
      end
    end

    // Stop bit — sample at midpoint.  Line must be high; flag framing error if not.
    Stop: begin
      if (sample_inc) begin
        if (sample_cnt == 4'd7) begin
          rx_valid_d   = rx_s1;          // Valid only if stop bit is high
          rx_error_d   = !rx_s1;         // Framing error if low
          state_d      = Idle;
        end else begin
          sample_cnt_d = sample_cnt + 4'd1;
        end
      end
    end

    default: begin
    end

  endcase
end

//------------------------------------------------------------------------------
// Outputs
//------------------------------------------------------------------------------
assign rx_data_o  = data;
assign rx_valid_o = rx_valid;
assign rx_error_o = rx_error;

endmodule

`default_nettype wire