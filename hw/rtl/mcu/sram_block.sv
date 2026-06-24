//------------------------------------------------------------------------------
// (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "defs.svh"

//------------------------------------------------------------------------------
// Infer a BSRAM instance
//------------------------------------------------------------------------------
module sram_block #(parameter MEM_FILE, 
                            WORD_ADDRESS_WIDTH)
(
  input `VAR logic clk_i,
  input `VAR logic rst_ni,
  input `VAR logic clk_en_i,
  input `VAR logic wrt_en_i,
  input `VAR logic[WORD_ADDRESS_WIDTH-1:0] addr_i,
  input `VAR logic[7:0] data_i,
  output logic[7:0] data_o
);

logic [7:0] bsram[1 << WORD_ADDRESS_WIDTH], data;

//------------------------------------------------------------------------------
// Load firmware into RAM
//------------------------------------------------------------------------------
initial begin
  if (MEM_FILE != "") $readmemh(MEM_FILE, bsram);  
end

always @(posedge clk_i) begin
  if(clk_en_i) begin
    if(wrt_en_i) bsram[addr_i] <= data_i;      
    data <= bsram[addr_i];        
  end
end

assign data_o = data;

endmodule

`default_nettype wire