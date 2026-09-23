`include "define.vh"
// CPU 顶层. 端口的方向 / 命名 / 位宽完全按
// ysyxSoC/ready-to-run/minirv/cpu-interface.md 的规范: clock / reset + SimpleBus
// pc / inst / ebreak / misalign 只给仿真环境看, 不再是端口, 由 sim_top 用层次引用读
module top(
	input  clock,
	input  reset,
	output        io_ifu_reqValid,
	output [31:0] io_ifu_addr,
	input         io_ifu_respValid,
	input  [31:0] io_ifu_rdata,
	output        io_lsu_reqValid,
	output [31:0] io_lsu_addr,
	output [1:0]  io_lsu_size,
	output        io_lsu_wen,
	output [31:0] io_lsu_wdata,
	output [3:0]  io_lsu_wmask,
	input         io_lsu_respValid,
	input  [31:0] io_lsu_rdata
);
	// ---- 观察信号: 只给仿真环境用, 不对外连接 ----
	wire [31:0] pc;        // pc_reg 输出的当前 PC
	wire [31:0] inst;      // IFU 输出的当前指令
	reg         ebreak;    // 执行到 ebreak 时置 1, 供仿真环境判断程序结束
	reg         misalign;  // lw/sw 地址未 4 字节对齐时置 1, 供仿真环境报错

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

	wire inst_is_load = (lsu_op == `LSU_LW) || (lsu_op == `LSU_LBU);
	wire lsu_done = lsu_busy && io_lsu_respValid;    
	wire commit   = (ifu_valid && !inst_is_load) || lsu_done;

	wire rf_we = gpr_we && (waddr != 5'd0) && commit;
	wire pc_we = commit;

	always @(posedge clock) begin
		if (reset) misalign <= 1'b0;
		else       misalign <= lsu_misalign;
	end

	always @(posedge clock) begin
		if (reset) begin
			ebreak <= 1'b0;
		end
		else if (is_ebreak && commit && !ebreak) begin
			ebreak <= 1'b1;
		end
	end

	import "DPI-C" function void sim_retire(input int pc, input int inst);
	always @(posedge clock) begin
		if (commit) sim_retire(pc, inst);
	end


	pc_reg u_pc_reg(
		.clk(clock),
		.rst(reset),
		.we(pc_we),
		.next_pc(next_pc),
		.pc(pc)
	);

	ifu u_ifu(
		.clk(clock),
		.rst(reset),
		.pc(pc),
		.inst(inst),
		.lsu_busy(lsu_busy),
		.ifu_valid(ifu_valid),
		.ifu_addr(io_ifu_addr),
		.ifu_reqValid(io_ifu_reqValid),
		.ifu_respValid(io_ifu_respValid),
		.ifu_rdata(io_ifu_rdata)
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
		.clk(clock),
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
		.clk(clock),
		.rst(reset),
		.valid(ifu_valid),
		.lsu_op(lsu_op),
		.addr(alu_result),
		.wdata(rdata2),
		.lsu_rdata(io_lsu_rdata),
		.rdata(mem_rdata),
		.lsu_busy(lsu_busy),
		.lsu_misalign(lsu_misalign),
		.lsu_addr(io_lsu_addr),
		.lsu_wdata(io_lsu_wdata),
		.lsu_wmask(io_lsu_wmask),
		.lsu_size(io_lsu_size),
		.lsu_wen(io_lsu_wen),
		.lsu_reqValid(io_lsu_reqValid),
		.lsu_respValid(io_lsu_respValid)
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
