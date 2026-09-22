module top(
	input  clk,
	input  rst,
	output [31:0] pc,      // 供仿真环境观察: 当前 PC
	output [31:0] inst,    // 供仿真环境观察: 当前指令
	output reg    ebreak,  // 程序执行到 ebreak 时置 1, 供仿真环境判断程序结束  
	output reg    misalign, // lw/sw 地址未 4 字节对齐时置 1, 供仿真环境报错
	
	output [31:0] ifu_raddr,
	input  [31:0] ifu_rdata,
	output [31:0] dmem_addr,
	output [31:0] dmem_wdata,
	output [3:0]  dmem_wmask,
	output        dmem_re,
	output        dmem_we,
	input  [31:0] dmem_rdata
);
	// ---- pc_reg <-> 数据通路 ----
	wire [31:0] next_pc;
	wire        pc_we;

	// ---- IFU ----
	wire        ifu_valid; // 本拍 IFU 给出的 inst 是有效指令

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

	// 取指占两拍, 只有 ifu_valid 的周期才真正有一条指令在执行;
	// 否则 idu 会拿 IFU 里的旧信息去译码, 必须屏蔽掉所有状态更新
	// x0 恒为 0: 写 0 号寄存器时把写使能屏蔽掉
	wire rf_we = gpr_we && (waddr != 5'd0) && ifu_valid;
	wire pc_we = ifu_valid;

	always @(posedge clk) begin
		if (rst) misalign <= 1'b0;
		else     misalign <= lsu_misalign; // lsu_misalign 已按 ifu_valid 屏蔽
	end

	always @(posedge clk) begin
		if (rst) begin
			ebreak <= 1'b0;
		end
		else if (is_ebreak && ifu_valid && !ebreak) begin
			ebreak <= 1'b1;
		end
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
		.valid(ifu_valid),
		.lsu_op(lsu_op),
		.addr(alu_result),
		.wdata(rdata2),
		.dmem_rdata(dmem_rdata),
		.rdata(mem_rdata),
		.lsu_misalign(lsu_misalign),
		.dmem_addr(dmem_addr),
		.dmem_wdata(dmem_wdata),
		.dmem_wmask(dmem_wmask),
		.dmem_re(dmem_re),
		.dmem_we(dmem_we)
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
