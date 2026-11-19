`include "define.vh"
module ysyx_26100296(
  input                     clock,
  input                     reset,
  output                    io_ifu_reqValid,
  output [`CPU_WIDTH-1:0]   io_ifu_addr,
  input                     io_ifu_respValid,
  input  [`CPU_WIDTH-1:0]   io_ifu_rdata,
  output                    io_lsu_reqValid,
  output [`CPU_WIDTH-1:0]   io_lsu_addr,
  output [1:0]              io_lsu_size,
  output                    io_lsu_wen,
  output [`CPU_WIDTH-1:0]   io_lsu_wdata,
  output [`CPU_WIDTH/8-1:0] io_lsu_wmask,
  input                     io_lsu_respValid,
  input  [`CPU_WIDTH-1:0]   io_lsu_rdata
);
  wire [`CPU_WIDTH-1:0] pc;
  wire [`CPU_WIDTH-1:0] inst;
  reg         misalign;

  wire [`CPU_WIDTH-1:0] pc_plus4;

  // ---- IFU ----
  wire        ifu_valid;

  // ---- LSU ----
  wire        lsu_busy;

  // ---- IDU -> EXU ----
  wire [`CPU_WIDTH-1:0] imm;
  wire [3:0]  alu_op;
  wire        alu_src1_sel;
  wire        alu_src2_sel;
  wire        is_jump;
  wire        is_jalr;
  wire        is_branch;
  wire        rt_src1_sel;

  // ---- IDU -> LSU ----
  wire [3:0]  lsu_op;

  // ---- IDU -> GPR / WBU ----
  wire [`REG_WIDTH-1:0] raddr1, raddr2, waddr;
  wire        gpr_we;
  wire [1:0]  wb_sel;
  wire        is_ebreak;
  wire [11:0] csr_addr;

  // ---- CSR -> WBU ----
  wire [`CPU_WIDTH-1:0] csr_rdata;

  // ---- GPR -> EXU / LSU ----
  wire [`CPU_WIDTH-1:0] rdata1, rdata2;

  // ---- EXU -> LSU / WBU / IFU ----
  wire [`CPU_WIDTH-1:0] alu_result;
  wire        redirect;
  wire [`CPU_WIDTH-1:0] redirect_target;

  // ---- LSU -> WBU ----
  wire [`CPU_WIDTH-1:0] mem_rdata;

  wire lsu_misalign;

  // ---- WBU -> GPR ----
  wire [`CPU_WIDTH-1:0] wb_data;

  wire inst_is_load = (lsu_op == `LSU_LB) || (lsu_op == `LSU_LBU) ||
                      (lsu_op == `LSU_LH) || (lsu_op == `LSU_LHU) ||
                      (lsu_op == `LSU_LW);
  wire lsu_done = lsu_busy && io_lsu_respValid;
  wire commit   = (ifu_valid && !inst_is_load) || lsu_done;

  wire rf_we = gpr_we && commit;
  wire pc_we = commit;

  always @(posedge clock) begin
    if (reset) misalign <= 1'b0;
    else       misalign <= lsu_misalign;
  end

  ysyx_26100296_ifu u_ifu(
    .clk_i(clock),
    .rst_i(reset),
    .pc_we_i(pc_we),
    .pc_o(pc),
    .redirect_i(redirect),
    .redirect_target_i(redirect_target),
    .pc_plus4_o(pc_plus4),
    .inst_o(inst),
    .lsu_busy_i(lsu_busy),
    .ifu_valid_o(ifu_valid),
    .ifu_addr_o(io_ifu_addr),
    .ifu_reqValid_o(io_ifu_reqValid),
    .ifu_respValid_i(io_ifu_respValid),
    .ifu_rdata_i(io_ifu_rdata)
  );

  ysyx_26100296_idu u_idu(
    .inst_i(inst),
    .rs1_o(raddr1),
    .rs2_o(raddr2),
    .rd_o(waddr),
    .gpr_we_o(gpr_we),
    .wb_sel_o(wb_sel),
    .imm_o(imm),
    .alu_op_o(alu_op),
    .alu_src1_sel_o(alu_src1_sel),
    .alu_src2_sel_o(alu_src2_sel),
    .is_jump_o(is_jump),
    .is_jalr_o(is_jalr),
    .is_branch_o(is_branch),
    .rt_src1_sel_o(rt_src1_sel),
    .lsu_op_o(lsu_op),
    .is_ebreak_o(is_ebreak),
    .csr_addr_o(csr_addr)
  );

  ysyx_26100296_gpr u_gpr(
    .clk_i(clock),
    .wdata_i(wb_data),
    .waddr_i(waddr),
    .wen_i(rf_we),
    .raddr1_i(raddr1),
    .raddr2_i(raddr2),
    .rdata1_o(rdata1),
    .rdata2_o(rdata2)
  );

  ysyx_26100296_exu u_exu(
    .pc_i(pc),
    .rdata1_i(rdata1),
    .rdata2_i(rdata2),
    .imm_i(imm),
    .alu_op_i(alu_op),
    .alu_src1_sel_i(alu_src1_sel),
    .alu_src2_sel_i(alu_src2_sel),
    .is_jump_i(is_jump),
    .is_jalr_i(is_jalr),
    .is_branch_i(is_branch),
    .rt_src1_sel_i(rt_src1_sel),
    .alu_result_o(alu_result),
    .redirect_o(redirect),
    .redirect_target_o(redirect_target)
  );

  ysyx_26100296_csr u_csr(
    .clk_i(clock),
    .rst_i(reset),
    .addr_i(csr_addr),
    .rdata_o(csr_rdata)
  );

  ysyx_26100296_lsu u_lsu(
    .clk_i(clock),
    .rst_i(reset),
    .valid_i(ifu_valid),
    .lsu_op_i(lsu_op),
    .addr_i(alu_result),
    .wdata_i(rdata2),
    .lsu_rdata_i(io_lsu_rdata),
    .rdata_o(mem_rdata),
    .lsu_busy_o(lsu_busy),
    .lsu_misalign_o(lsu_misalign),
    .lsu_addr_o(io_lsu_addr),
    .lsu_wdata_o(io_lsu_wdata),
    .lsu_wmask_o(io_lsu_wmask),
    .lsu_size_o(io_lsu_size),
    .lsu_wen_o(io_lsu_wen),
    .lsu_reqValid_o(io_lsu_reqValid),
    .lsu_respValid_i(io_lsu_respValid)
  );

  ysyx_26100296_wbu u_wbu(
    .pc_plus4_i(pc_plus4),
    .wb_sel_i(wb_sel),
    .alu_result_i(alu_result),
    .mem_rdata_i(mem_rdata),
    .csr_rdata_i(csr_rdata),
    .wb_data_o(wb_data)
  );


`ifndef SYNTHESIS
`ifdef __ICARUS__
  always @(posedge clock) if (commit) $sim_retire(pc, inst);
`else
  import "DPI-C" function void sim_retire(input int pc, input int inst);
  import "DPI-C" function void ebreak(input int code);
  always @(posedge clock) begin
    if (commit) sim_retire(pc, inst);
  end
`endif

  wire [`CPU_WIDTH-1:0] a0 = u_gpr.rf[10];

  always @(posedge clock) begin
    if (!reset && is_ebreak && commit) begin
      if (a0 == `CPU_WIDTH'd0) $display("EBREAK: GOOD TRAP");
      else             $display("EBREAK: BAD TRAP (a0 = %0d)", a0);
`ifdef __ICARUS__
      $ebreak(a0);
`else
      ebreak(a0);
`endif
    end
  end

`endif

endmodule
