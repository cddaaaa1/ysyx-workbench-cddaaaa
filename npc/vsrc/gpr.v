`include "define.vh"

module ysyx_26100296_gpr (
  input                   clk_i,
  input  [`CPU_WIDTH-1:0] wdata_i,
  input  [`REG_WIDTH-1:0] waddr_i,
  input                   wen_i,

  input  [`REG_WIDTH-1:0] raddr1_i,
  input  [`REG_WIDTH-1:0] raddr2_i,
  output [`CPU_WIDTH-1:0] rdata1_o,
  output [`CPU_WIDTH-1:0] rdata2_o
);
  reg [`CPU_WIDTH-1:0] rf [2**`REG_WIDTH-1:0];
  always @(posedge clk_i) begin
    if (wen_i && (waddr_i !=`REG_WIDTH'b0)) rf[waddr_i] <= wdata_i;
  end

  assign rdata1_o = (raddr1_i == 0) ? {`CPU_WIDTH{1'b0}} : rf[raddr1_i];
  assign rdata2_o = (raddr2_i == 0) ? {`CPU_WIDTH{1'b0}} : rf[raddr2_i];
endmodule

