`include "define.vh"
module ysyx_22040000(
	input  clock,
	input  reset
`ifdef YSYXSOC
    ,output        io_ifu_reqValid
    ,output [31:0] io_ifu_addr
    ,input         io_ifu_respValid
    ,input  [31:0] io_ifu_rdata
    ,output        io_lsu_reqValid
    ,output [31:0] io_lsu_addr
    ,output [1:0]  io_lsu_size
    ,output        io_lsu_wen
    ,output [31:0] io_lsu_wdata
    ,output [3:0]  io_lsu_wmask
    ,input         io_lsu_respValid
    ,input  [31:0] io_lsu_rdata
`endif
);

`ifndef YSYXSOC
	wire        io_ifu_reqValid;
	wire [31:0] io_ifu_addr;
	wire        io_ifu_respValid;
	wire [31:0] io_ifu_rdata;
	wire        io_lsu_reqValid;
	wire [31:0] io_lsu_addr;
	wire [1:0]  io_lsu_size;
	wire        io_lsu_wen;
	wire [31:0] io_lsu_wdata;
	wire [3:0]  io_lsu_wmask;
	wire        io_lsu_respValid;
	wire [31:0] io_lsu_rdata;
`endif
	wire [31:0] pc;        
	wire [31:0] inst;         
	reg         misalign; 

	// ---- pc_reg <-> 数据通路 ----
	wire [31:0] next_pc;

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
	wire [11:0] csr_addr;

	// ---- CSR -> WBU ----
	wire [31:0] csr_rdata;

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

	ysyx_22040000_pc_reg u_pc_reg(
		.clk(clock),
		.rst(reset),
		.we(pc_we),
		.next_pc(next_pc),
		.pc(pc)
	);

	ysyx_22040000_ifu u_ifu(
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

	ysyx_22040000_idu u_idu(
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
		.is_ebreak(is_ebreak),
		.csr_addr(csr_addr)
	);

	ysyx_22040000_gpr u_gpr(
		.clk(clock),
		.wdata(wb_data),
		.waddr(waddr),
		.wen(rf_we),
		.raddr1(raddr1),
		.raddr2(raddr2),
		.rdata1(rdata1),
		.rdata2(rdata2)
	);

	ysyx_22040000_exu u_exu(
		.rdata1(rdata1),
		.rdata2(rdata2),
		.imm(imm),
		.alu_op(alu_op),
		.is_jalr(is_jalr),
		.alu_result(alu_result),
		.jump(jump),
		.jump_target(jump_target)
	);

	ysyx_22040000_csr u_csr(
		.clk(clock),
		.rst(reset),
		.addr(csr_addr),
		.rdata(csr_rdata)
	);

	ysyx_22040000_lsu u_lsu(
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

	ysyx_22040000_wbu u_wbu(
		.pc(pc),
		.wb_sel(wb_sel),
		.alu_result(alu_result),
		.mem_rdata(mem_rdata),
		.csr_rdata(csr_rdata),
		.jump(jump),
		.jump_target(jump_target),
		.wb_data(wb_data),
		.next_pc(next_pc)
	);

`ifndef YSYXSOC
    dpic_mem u_dpic_mem (
        .clk(clock),
        .rst(reset),
        .ifu_addr(io_ifu_addr),
        .ifu_reqValid(io_ifu_reqValid),
        .ifu_respValid(io_ifu_respValid),
        .ifu_rdata(io_ifu_rdata),
        .lsu_addr(io_lsu_addr),
        .lsu_wdata(io_lsu_wdata),
        .lsu_wmask(io_lsu_wmask),
        .lsu_size(io_lsu_size),
        .lsu_wen(io_lsu_wen),
        .lsu_reqValid(io_lsu_reqValid),
        .lsu_respValid(io_lsu_respValid),
        .lsu_rdata(io_lsu_rdata)
    );
`endif

`ifndef SYNTHESIS

	import "DPI-C" function void sim_retire(input int pc, input int inst);
	always @(posedge clock) begin
		if (commit) sim_retire(pc, inst);
	end

	wire [31:0] a0 = u_gpr.rf[10];

	always @(posedge clock) begin
		if (is_ebreak && commit) begin
			if (a0 == 32'd0) $display("EBREAK: GOOD TRAP");
			else             $display("EBREAK: BAD TRAP (a0 = %0d)", a0);
			$finish;
		end
	end

`endif

endmodule
