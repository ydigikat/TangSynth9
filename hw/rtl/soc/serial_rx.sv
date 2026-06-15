//------------------------------------------------------------------------------
// Jason Wilden 2026
//------------------------------------------------------------------------------

`default_nettype none
`include "../../common_defs.svh"


module serial_rx 
(
  input `VAR  logic       clk_i,
  input `VAR  logic       rst_ni,
  input `VAR  logic[10:0] div_i,
  input `VAR  logic       rx_i,
  output      logic       rx_done_o,
  output      logic[7:0]  data_o
);

//---------------------------------------------------------------------------
// State registers
//---------------------------------------------------------------------------
uart_state_t state, state_d;
logic [10:0] sample_div, sample_div_d;
logic [3:0] sample_cnt, sample_cnt_d;
logic[2:0] bit_cnt, bit_cnt_d;
logic [7:0] data, data_d;

always_ff @(posedge clk_i) begin
  if (!rst_ni) begin
    state <= Idle;
    sample_div <= 0;
    sample_cnt <= 0;
    bit_cnt <= 0;
    data <= 0;      
  end else begin
    state <= state_d;
    sample_div <= sample_div_d;
    sample_cnt <= sample_cnt_d;
    bit_cnt <= bit_cnt_d;
    data <= data_d;          
  end
end

//------------------------------------------------------------------------------
// Sample rate divider
//------------------------------------------------------------------------------
logic sample_inc;
assign sample_div_d = (sample_div == div_i) ? 11'd0 : sample_div + 11'd1;
assign sample_inc = (sample_div == div_i);

//------------------------------------------------------------------------------
// Next-state / Output logic
//------------------------------------------------------------------------------
always_comb begin
  // Default hold
  state_d = state;
  rx_done_o = 1'b0;
  sample_cnt_d = sample_cnt;
  bit_cnt_d = bit_cnt;
  data_d = data;

  unique case (state)
    // Idle until rx line goes low
    Idle: begin
      if (!rx_i) begin
        state_d = Start;
        sample_cnt_d = 0;
      end
    end
    // Walk 7 samples into the start bit, this sets us up so that adding
    // 15 samples will place us in the middle of each of the subsequent
    // data and stop bits. 
    Start:begin  
      if (sample_inc) begin        
        if (sample_cnt == 7) begin
          state_d = Data;
          sample_cnt_d = 0;
          bit_cnt_d = 0;
        end else begin
          sample_cnt_d = sample_cnt + 1'd1;
        end
      end
    end
    // Shift in 8 data bits
    Data: begin      
      if (sample_inc) begin
        if (sample_cnt == 15) begin
          sample_cnt_d = 0;
          data_o = {rx_i, data[7:1]};
          if (bit_cnt == 7) state_d = Stop;
          else bit_cnt_d = bit_cnt + 1'd1;          
        end else begin
          sample_cnt_d = sample_cnt + 1'd1;
        end
      end
    end
    // Wait for the single stop bit to pass before indicating
    // that we're done and returning to Idle state.
    Stop: begin  
      if (sample_inc) begin
        if (sample_cnt == 15) begin
          state_d = Idle;
          rx_done_o = 1'd1;
        end else begin
          sample_cnt_d = sample_cnt + 1'd1;
        end
      end
    end

    default: begin
    end

  endcase
end

//---------------------------------------------------------------------------
// Outputs
//---------------------------------------------------------------------------
assign data_o = data;

endmodule

`default_nettype wire