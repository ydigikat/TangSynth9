//------------------------------------------------------------------------------
// (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "../defs.svh"

module test_tone (
    input `VAR logic clk_i,
    input `VAR logic rst_ni,
    input `VAR logic[15:0] fcw_i,
    input `VAR logic sample_req_i,
    output logic [31:0] sample_o

);
  //------------------------------------------------------------------------------
  // State registers
  //------------------------------------------------------------------------------
  always_ff @(posedge clk_i) begin
    if(~rst_ni) begin
      acc <= 15'd0;
      sample <= 31'd0;
    end else begin
      acc <= acc_next;
      sample <= sample_next;
    end
  end


//------------------------------------------------------------------------------
// Next state logic
//------------------------------------------------------------------------------
logic [15:0] acc, acc_next;
logic [31:0] sample, sample_next;

always_comb begin
  acc_next = acc;
  sample_next = sample;

  if(sample_req_i) begin
    acc_next = acc + fcw_i;    
    sample_next = {acc_next, acc_next};
  end
end

//------------------------------------------------------------------------------
// Output logic
//------------------------------------------------------------------------------
assign sample_o = sample;

endmodule

`default_nettype wire

