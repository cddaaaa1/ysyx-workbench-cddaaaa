`include "define.vh"
module top(
	input  clk,
	input  rst,
	output [31:0] pc,      // 供仿真环境观察: 当前 PC
	output [31:0] inst,    // 供仿真环境观察: 当前指令
	output reg    ebreak,  // 程序执行到 ebreak 时置 1, 供仿真环境判断程序结束  
	output reg    misalign, // lw/sw 地址未 4 字节对齐时置 1, 供仿真环境报错
	
	output [31:0] ifu_raddr,
	input  [31:0] ifu_rdata,
	output [31:0] lsu_addr,
	output [31:0] lsu_wdata,
	output [3:0]  lsu_wmask,
	output        lsu_re,
	output        lsu_wen,
	input  [31:0] lsu_rdata
);
	// ---- pc_reg <-> 数据通路 ----
	wire [31:0] next_pc;
	wire        pc_we;

	// ---- IFU ----
	wire        ifu_valid; // 本拍 IFU 给出的 inst 是有效指令

	// ---- LSU ----
	wire        lsu_busy;  // LSU 正在等存储器返回数据(load 多花的那一拍)

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
	wire        is_ebreak;

	// ---- GPR -> EXU / LSU ----
	wire [31:0] rdata1, rdata2;

	// ---- EXU -> LSU / WBU ----
	wire [31:0] alu_result;
	wire        jump;
	wire [31:0] jump_target;

	// ---- LSU -> WBU ----
	wire [31:0] mem_rdata;

	// ---- LSU -> 仿真环境 ----
	wire lsu_misalign;

	// ---- WBU -> GPR ----
	wire [31:0] wb_data;

	// 一条指令真正执行完毕(可以提交/退休)的那一拍:
	//   - 非 load 指令: IFU 的 wait 拍
	//   - load 指令:   还要再等一拍, LSU 才把读出的数据送回来
	wire inst_is_load = (lsu_op == `LSU_LW) || (lsu_op == `LSU_LBU);
	wire commit = (ifu_valid && !inst_is_load) || lsu_busy;

	// 只有 commit 的周期才真正有一条指令执行完毕;
	// 否则 idu 会拿 IFU 里的旧信息去译码, 必须屏蔽掉所有状态更新
	// x0 恒为 0: 写 0 号寄存器时把写使能屏蔽掉
	wire rf_we = gpr_we && (waddr != 5'd0) && commit;
	wire pc_we = commit;

	always @(posedge clk) begin
		if (rst) misalign <= 1'b0;
		else     misalign <= lsu_misalign; // lsu_misalign 已按 ifu_valid 屏蔽
	end

	always @(posedge clk) begin
		if (rst) begin
			ebreak <= 1'b0;
		end
		else if (is_ebreak && commit && !ebreak) begin
			ebreak <= 1'b1;
		end
	end

	// 指令退休时通过 DPI-C 通知仿真环境; 此刻 pc / inst 仍是退休那条指令的
	import "DPI-C" function void sim_retire(input int pc, input int inst);
	always @(posedge clk) begin
		if (commit) sim_retire(pc, inst);
	end


	pc_reg u_pc_reg(
		.clk(clk),
		.rst(rst),
		.we(pc_we),
		.next_pc(next_pc),
		.pc(pc)
	);

	ifu u_ifu(
		.clk(clk),
		.rst(rst),
		.pc(pc),
		.inst(inst),
		.lsu_busy(lsu_busy),
		.ifu_valid(ifu_valid),
		.ifu_raddr(ifu_raddr),
		.ifu_rdata(ifu_rdata)
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
		.is_ebreak(is_ebreak)
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
		.clk(clk),
		.rst(rst),
		.valid(ifu_valid),
		.lsu_op(lsu_op),
		.addr(alu_result),
		.wdata(rdata2),
		.lsu_rdata(lsu_rdata),
		.rdata(mem_rdata),
		.lsu_busy(lsu_busy),
		.lsu_misalign(lsu_misalign),
		.lsu_addr(lsu_addr),
		.lsu_wdata(lsu_wdata),
		.lsu_wmask(lsu_wmask),
		.lsu_re(lsu_re),
		.lsu_wen(lsu_wen)
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
