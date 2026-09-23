// RV32I: 32 个 32 位通用寄存器, 2 读 1 写
// x0 恒为 0: 读侧在 assign 里处理, 写侧由外部屏蔽 wen (waddr != 0)
module ysyx_22040000_gpr #(ADDR_WIDTH = 5, DATA_WIDTH = 32) (
  input clk,
  input [DATA_WIDTH-1:0] wdata,
  input [ADDR_WIDTH-1:0] waddr,
  input wen,
  // 读端口 (组合读, 供 EXU 使用)
  input  [ADDR_WIDTH-1:0] raddr1,
  input  [ADDR_WIDTH-1:0] raddr2,
  output [DATA_WIDTH-1:0] rdata1,
  output [DATA_WIDTH-1:0] rdata2
);
  reg [DATA_WIDTH-1:0] rf [2**ADDR_WIDTH-1:0];
  always @(posedge clk) begin
    if (wen) rf[waddr] <= wdata;
  end

  // x0 恒为 0: 读侧在这里处理, 写侧由外部屏蔽 wen (waddr != 0)
  assign rdata1 = (raddr1 == 0) ? {DATA_WIDTH{1'b0}} : rf[raddr1];
  assign rdata2 = (raddr2 == 0) ? {DATA_WIDTH{1'b0}} : rf[raddr2];
endmodule

