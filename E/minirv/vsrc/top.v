// minirv NPC 顶层: 只负责模块互连
// 需要例化: pc_reg / gpr (状态) + ifu / idu / exu / lsu / wbu (纯组合)
//
// 注意: 接到 gpr 的写使能要屏蔽 x0, 例如
//   wire rf_we = gpr_we && (waddr != 4'd0);
module top(
	input  clk,
	input  rst,
	output [31:0] pc,      // 供仿真环境观察: 当前 PC
	output [31:0] inst,    // 供仿真环境观察: 当前指令
	output        ebreak   // 程序执行到 ebreak 时置 1, 供仿真环境判断程序结束
);
	// ---- pc_reg <-> 数据通路 ----
	wire [31:0] next_pc;

	// ---- IDU -> EXU ----
	wire [31:0] imm;
	wire [3:0]  alu_op;
	wire        is_jalr;

	// ---- IDU -> LSU ----
	wire [2:0]  lsu_op;

	// ---- IDU -> GPR / WBU ----
	wire [4:0]  raddr1, raddr2, waddr;
	wire        gpr_we;
	wire [1:0]  wb_sel;

	// ---- GPR -> EXU / LSU ----
	wire [31:0] rdata1, rdata2;

	// ---- EXU -> LSU / WBU ----
	wire [31:0] alu_result;
	wire        jump;
	wire [31:0] jump_target;

	// ---- LSU -> WBU ----
	wire [31:0] mem_rdata;

	// ---- WBU -> GPR ----
	wire [31:0] wb_data;

	// x0 恒为 0: 写 0 号寄存器时把写使能屏蔽掉
	wire rf_we = gpr_we && (waddr != 5'd0);

	pc_reg u_pc_reg(
		.clk(clk),
		.rst(rst),
		.next_pc(next_pc),
		.pc(pc)
	);

	ifu u_ifu(
		.pc(pc),
		.inst(inst)
	);

	idu u_idu(
		.inst(inst),
		.raddr1(raddr1),
		.raddr2(raddr2),
		.waddr(waddr),
		.gpr_we(gpr_we),
		.wb_sel(wb_sel),
		.imm(imm),
		.alu_op(alu_op),
		.is_jalr(is_jalr),
		.lsu_op(lsu_op),
		.is_ebreak(ebreak)
	);

	gpr u_gpr(
		.clk(clk),
		.wdata(wb_data),
		.waddr(waddr),
		.wen(rf_we),
		.raddr1(raddr1),
		.raddr2(raddr2),
		.rdata1(rdata1),
		.rdata2(rdata2)
	);

	exu u_exu(
		.rdata1(rdata1),
		.rdata2(rdata2),
		.imm(imm),
		.alu_op(alu_op),
		.is_jalr(is_jalr),
		.alu_result(alu_result),
		.jump(jump),
		.jump_target(jump_target)
	);

	lsu u_lsu(
		.lsu_op(lsu_op),
		.addr(alu_result),
		.wdata(rdata2),
		.rdata(mem_rdata)
	);

	wbu u_wbu(
		.pc(pc),
		.wb_sel(wb_sel),
		.alu_result(alu_result),
		.mem_rdata(mem_rdata),
		.jump(jump),
		.jump_target(jump_target),
		.wb_data(wb_data),
		.next_pc(next_pc)
	);

endmodule
