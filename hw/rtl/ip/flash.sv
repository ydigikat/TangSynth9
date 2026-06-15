//------------------------------------------------------------------------------
// Estella (c) Jason Wilden 2026
//------------------------------------------------------------------------------
`default_nettype none

module flash(
  input `VAR logic[31:0] data_i,
  input `VAR logic[8:0] xaddr_i,
  input `VAR logic[5:0] yaddr_i,
  input `VAR logic      xen_i,
  input `VAR logic      yen_i,
  input `VAR logic      sen_i,
  input `VAR logic      prog_i,
  input `VAR logic      erase_i,
  input `VAR logic      nvstr_i,
  output     logic[31:0] data_o);

FLASH608K u_flash(
  .DOUT(data_o),
  .XE(xen_i),
  .YE(yen_i),
  .SE(sen_i),
  .PROG(prog_i),
  .NVSTR(nvstr_i),
  .ERASE(erase_i),
  .XADDR(xaddr_i),
  .YADDR(yaddr_i),
  .DIN(data_i)
);

endmodule

`default_nettype wire