`include "define.vh"
module ysyx_26100296_ifu(
  input  clk_i,
  input  rst_i,
  input  [`CPU_WIDTH-1:0] pc_i,
  input  [`CPU_WIDTH-1:0] ifu_rdata_i,
  input         lsu_busy_i,
  output [`CPU_WIDTH-1:0] inst_o,
  output        ifu_valid_o,
  output [`CPU_WIDTH-1:0] ifu_addr_o,
  input         ifu_respValid_i,
  output        ifu_reqValid_o
);
  reg state;

  always @(posedge clk_i) begin
    if (rst_i) begin
      state <= `IFU_IDLE;
    end
    else if (state == `IFU_IDLE) state <= ifu_reqValid_o ? `IFU_WAIT : `IFU_IDLE;
    else                         state <= ifu_respValid_i ? `IFU_IDLE : `IFU_WAIT;
  end

  assign ifu_reqValid_o = (state == `IFU_IDLE) && !lsu_busy_i;
  assign ifu_valid_o = (state == `IFU_WAIT) && ifu_respValid_i;

  assign ifu_addr_o = pc_i;
  assign inst_o = ifu_rdata_i;
endmodule
