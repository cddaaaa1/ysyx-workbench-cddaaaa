`include "define.vh"
module ysyx_26100296_ifu(
  input                   clk_i,
  input                   rst_i,
  input                   pc_we_i,
  input                   redirect_i,
  input  [`CPU_WIDTH-1:0] redirect_target_i,
  output [`CPU_WIDTH-1:0] pc_o,
  output [`CPU_WIDTH-1:0] pc_plus4_o,
  input  [`CPU_WIDTH-1:0] ifu_rdata_i,
  input                   lsu_busy_i,
  output [`CPU_WIDTH-1:0] inst_o,
  output                  ifu_valid_o,
  output [`CPU_WIDTH-1:0] ifu_addr_o,
  input                   ifu_respValid_i,
  output                  ifu_reqValid_o
);
  reg state;
  wire [`CPU_WIDTH-1:0] next_pc;

  Reg #(
    .WIDTH(`CPU_WIDTH),
    .RESET_VAL(`PC_RESET)
  ) pc_reg (
    .clk(clk_i),
    .rst(rst_i),
    .din(next_pc),
    .dout(pc_o),
    .wen(pc_we_i)
  );

  always @(posedge clk_i) begin
    if (rst_i) begin
      state <= `IFU_IDLE;
    end
    else if (state == `IFU_IDLE) state <= ifu_reqValid_o ? `IFU_WAIT : `IFU_IDLE;
    else                         state <= ifu_respValid_i ? `IFU_IDLE : `IFU_WAIT;
  end

  assign ifu_reqValid_o = (state == `IFU_IDLE) && !lsu_busy_i;
  assign ifu_valid_o = (state == `IFU_WAIT) && ifu_respValid_i;

  assign ifu_addr_o = pc_o;
  assign pc_plus4_o = pc_o + `CPU_WIDTH'd4;
  assign inst_o = ifu_rdata_i;

  MuxKey #(2, 1, `CPU_WIDTH) u_next_pc (
    .out(next_pc), .key(redirect_i),
    .lut({1'b0, pc_plus4_o,
          1'b1, redirect_target_i})
  );
endmodule
