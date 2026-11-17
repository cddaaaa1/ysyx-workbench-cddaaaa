`include "define.vh"
module ysyx_26100296_exu(
  input  [`CPU_WIDTH-1:0] pc_i,
  input  [`CPU_WIDTH-1:0] rdata1_i,
  input  [`CPU_WIDTH-1:0] rdata2_i,
  input  [`CPU_WIDTH-1:0] imm_i,
  input  [3:0]  alu_op_i,
  input         alu_src1_sel_i,
  input         alu_src2_sel_i,
  input         is_jump_i,
  input         is_jalr_i,
  input         is_branch_i,
  input         rt_src1_sel_i,
  output [`CPU_WIDTH-1:0] alu_result_o,
  output        redirect_o,
  output [`CPU_WIDTH-1:0] redirect_target_o
);

  wire [`CPU_WIDTH-1:0] alu_src1 = (alu_src1_sel_i == `ALU_SRC1_RS1) ? rdata1_i : pc_i;
  wire [`CPU_WIDTH-1:0] alu_src2 = (alu_src2_sel_i == `ALU_SRC2_RS2) ? rdata2_i : imm_i;
  wire [`CPU_WIDTH-1:0] rt_src1 = (rt_src1_sel_i == `RT_SRC1_RS1) ? rdata1_i : pc_i;
  wire [`CPU_WIDTH-1:0] rt_sum = rt_src1 + imm_i;
  wire branch_taken;
  wire alu_cmp_result;

  ysyx_26100296_alu u_alu(
    .a_i(alu_src1),
    .b_i(alu_src2),
    .op_i(alu_op_i),
    .result_o(alu_result_o),
    .cmp_result_o(alu_cmp_result)
  );

  assign branch_taken = is_branch_i && alu_cmp_result;
  assign redirect_target_o = is_jalr_i ? {rt_sum[`CPU_WIDTH-1:1], 1'b0} : rt_sum;
  assign redirect_o = is_jump_i || branch_taken;

endmodule

