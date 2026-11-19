`include "define.vh"

module ysyx_26100296_lsu(
  input                         clk_i,
  input                         rst_i,
  input                         valid_i,
  input      [3:0]              lsu_op_i,
  input      [`CPU_WIDTH-1:0]   addr_i,
  input      [`CPU_WIDTH-1:0]   wdata_i,
  input      [`CPU_WIDTH-1:0]   lsu_rdata_i,
  output reg [`CPU_WIDTH-1:0]   rdata_o,
  output                        lsu_busy_o,
  output                        lsu_misalign_o,
  output     [`CPU_WIDTH-1:0]   lsu_addr_o,
  output reg [`CPU_WIDTH-1:0]   lsu_wdata_o,
  output reg [`CPU_WIDTH/8-1:0] lsu_wmask_o,
  output reg [1:0]              lsu_size_o,
  output                        lsu_wen_o,
  output                        lsu_reqValid_o,
  input                         lsu_respValid_i
);
  reg state;

  wire is_load = valid_i && (lsu_op_i == `LSU_LB || lsu_op_i == `LSU_LBU ||
                            lsu_op_i == `LSU_LH || lsu_op_i == `LSU_LHU ||
                            lsu_op_i == `LSU_LW);
  wire is_store = valid_i && (lsu_op_i == `LSU_SB || lsu_op_i == `LSU_SH || lsu_op_i == `LSU_SW);
  wire is_mem = is_load || is_store;

  wire [15:0] rdata_half = addr_i[1] ? lsu_rdata_i[31:16] : lsu_rdata_i[15:0];
  wire [7:0] rdata_byte = addr_i[0] ? rdata_half[15:8] : rdata_half[7:0];

  always @(posedge clk_i) begin
    if (rst_i) state <= `LSU_IDLE;
    else if (state == `LSU_IDLE) state <= is_load ? `LSU_WAIT : `LSU_IDLE;
    else                         state <= lsu_respValid_i ? `LSU_IDLE : `LSU_WAIT;
  end

  assign lsu_reqValid_o = (state == `LSU_IDLE) && is_mem;
  assign lsu_busy_o     = (state == `LSU_WAIT);
  assign lsu_addr_o     = is_mem ? addr_i : {`CPU_WIDTH{1'b0}};
  assign lsu_wen_o      = is_store;

  always @* begin
    lsu_size_o  = 2'b00;
    lsu_wdata_o = {`CPU_WIDTH{1'b0}};
    lsu_wmask_o = {(`CPU_WIDTH/8){1'b0}};
    rdata_o     = {`CPU_WIDTH{1'b0}};

    case (lsu_op_i)
      `LSU_SB: begin
        lsu_size_o  = 2'b00;
        lsu_wdata_o = wdata_i << (addr_i[1:0] * 8);
        lsu_wmask_o = {{(`CPU_WIDTH/8-1){1'b0}}, 1'b1} << addr_i[1:0];
      end
      `LSU_SH: begin
        lsu_size_o  = 2'b01;
        lsu_wdata_o = wdata_i << (addr_i[1] * 16);
        lsu_wmask_o = {{(`CPU_WIDTH/8-2){1'b0}}, 2'b11} << (addr_i[1] * 2);
      end
      `LSU_SW: begin
        lsu_size_o  = 2'b10;
        lsu_wdata_o = wdata_i;
        lsu_wmask_o = {(`CPU_WIDTH/8){1'b1}};
      end
      `LSU_LB: begin
        lsu_size_o = 2'b00;
        rdata_o = {{(`CPU_WIDTH-8){rdata_byte[7]}}, rdata_byte};
      end
      `LSU_LBU: begin
        lsu_size_o = 2'b00;
        rdata_o = {{(`CPU_WIDTH-8){1'b0}}, rdata_byte};
      end
      `LSU_LHU: begin
        lsu_size_o = 2'b01;
        rdata_o = {{(`CPU_WIDTH-16){1'b0}}, rdata_half};
      end
      `LSU_LH: begin
        lsu_size_o = 2'b01;
        rdata_o = {{(`CPU_WIDTH-16){rdata_half[15]}}, rdata_half};
      end
      `LSU_LW: begin
        lsu_size_o = 2'b10;
        rdata_o = lsu_rdata_i;
      end
      default: begin end
    endcase
  end

  assign lsu_misalign_o = valid_i &&
    (((lsu_op_i == `LSU_LW || lsu_op_i == `LSU_SW) && addr_i[1:0] != 2'b0) ||
     ((lsu_op_i == `LSU_LH || lsu_op_i == `LSU_LHU || lsu_op_i == `LSU_SH) && addr_i[0]));
endmodule
