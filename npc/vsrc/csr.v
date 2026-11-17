`include "define.vh"
module ysyx_26100296_csr(
  input         clk_i,
  input         rst_i,
  input  [11:0] addr_i,
  output [`CPU_WIDTH-1:0] rdata_o
);

  wire [`CPU_WIDTH-1:0] mvendorid;
  wire [`CPU_WIDTH-1:0] marchid;
  wire [63:0] mcycle;

  stdreg #(
    .WIDTH    (`CPU_WIDTH),
    .RESET_VAL(`MVENDORID_VAL)
  ) reg_mvendorid (
    .clk_i  (clk_i),
    .rst_i  (rst_i),
    .wen_i  (1'b0),
    .din_i  ({`CPU_WIDTH{1'b0}}),
    .dout_o (mvendorid)
  );

  stdreg #(
    .WIDTH    (`CPU_WIDTH),
    .RESET_VAL(`MARCHID_VAL)
  ) reg_marchid (
    .clk_i  (clk_i),
    .rst_i  (rst_i),
    .wen_i  (1'b0),
    .din_i  ({`CPU_WIDTH{1'b0}}),
    .dout_o (marchid)
  );

  stdreg #(
    .WIDTH    (64),
    .RESET_VAL(64'b0)
  ) reg_mcycle (
    .clk_i  (clk_i),
    .rst_i  (rst_i),
    .wen_i  (1'b1),
    .din_i  (mcycle + 64'd1),
    .dout_o (mcycle)
  );

  assign rdata_o = (addr_i == `CSR_MVENDORID) ? mvendorid     :
    (addr_i == `CSR_MARCHID)   ? marchid       :
    (addr_i == `CSR_MCYCLE)    ? mcycle[31:0]   :
    (addr_i == `CSR_MCYCLEH)   ? mcycle[63:32] :
    {`CPU_WIDTH{1'b0}};

endmodule
