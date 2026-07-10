//------------------------------------------------------------------------------
// (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none
`include "../defs.svh"

module midi(
  input `VAR  logic   clk_i,
  input `VAR  logic   rst_ni,

  input  `VAR logic        rx_i,
  output      logic        irq_o,

  input  `VAR logic        select_i,
  output      logic        mem_ready_o,
  input  `VAR logic [3:0]  mem_wstrb_i,
  input  `VAR logic [31:0] mem_addr_i,
  input  `VAR logic [31:0] mem_wdata_i,
  output      logic [31:0] mem_rdata_o
);


//------------------------------------------------------------------------------
// Register offsets  (word-addressed via addr[5:2])
//------------------------------------------------------------------------------
localparam CR = 8'h00;   // Control  : [10:0] baud divisor
localparam SR = 8'h01;   // Status   : [0] rx_valid, [1] framing error
localparam RD = 8'h02;   // RX data  : [7:0] received byte

//------------------------------------------------------------------------------
// Decode
//------------------------------------------------------------------------------
logic wr_divisor, rd_status, rd_data;

assign wr_divisor = (|mem_wstrb_i && select_i && (mem_addr_i[5:2] == CR));
assign rd_status  = ( select_i && (mem_addr_i[5:2] == SR));
assign rd_data    = ( select_i && (mem_addr_i[5:2] == RD));

//------------------------------------------------------------------------------
// Registers
//------------------------------------------------------------------------------
logic [10:0] div;
logic        mem_ready;

// UART RX signals
logic [7:0]  rx_data;
logic        rx_valid;
logic        rx_error;

// Latched status — cleared on read of SR
logic        valid_lat, error_lat;

//------------------------------------------------------------------------------
// Process
//------------------------------------------------------------------------------
always_ff @(posedge clk_i) begin
  if (!rst_ni) begin
    div       <= '0;
    valid_lat <= 1'b0;
    error_lat <= 1'b0;
    mem_ready <= 1'b0;
  end else begin

    // Write divisor
    if (wr_divisor) div <= mem_wdata_i[10:0];

    // Latch incoming byte status; clear on SR read
    if (rx_valid) valid_lat <= 1'b1;
    if (rx_error) error_lat <= 1'b1;
    if (rd_status) begin
      valid_lat <= rx_valid;  // Re-arm 
      error_lat <= rx_error;
    end

    mem_ready <= select_i;
  end
end

//------------------------------------------------------------------------------
// Serial RX core
//------------------------------------------------------------------------------
serial_rx u_rx (
  .clk_i    (clk_i),
  .rst_ni   (rst_ni),
  .div_i    (div),
  .rx_i     (rx_i),
  .rx_data_o(rx_data),
  .rx_valid_o(rx_valid),
  .rx_error_o(rx_error)
);


//------------------------------------------------------------------------------
// Outputs
//------------------------------------------------------------------------------

// IRQ fires on every valid received byte (pulses for one cycle, matching rx_valid)
assign irq_o       = rx_valid;

assign mem_rdata_o = rd_data   ? {24'h0, rx_data}:
                     rd_status ? {30'h0, error_lat, valid_lat}:
                     32'h0;

assign mem_ready_o = mem_ready;

endmodule


`default_nettype wire