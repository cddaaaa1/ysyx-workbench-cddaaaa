module top(
	input        clk,
	input        rst,
	output [7:0] pc,
	output [7:0] r0,
	output [7:0] r1,
	output [7:0] r2,
	output [7:0] r3
);

	wire [7:0] next_pc;
	wire [7:0] instruction;
	wire [7:0] pc_plus_one;
	wire [7:0] branch_addr;
	wire [7:0] immediate;
	wire [7:0] rdata1;
	wire [7:0] rdata2;
	wire [7:0] alu_result;
	wire [7:0] writeback_data;
	wire [1:0] raddr1;
	wire [1:0] raddr2;
	wire [1:0] waddr;
	wire [1:0] alu_op;
	wire alu_not_equal;
	wire gpr_we;
	wire branch_taken;
	wire [1:0] writeback_sel;

	pc_reg u_pc_reg(
		.clk(clk),
		.rst(rst),
		.next_pc(next_pc),
		.pc(pc)
	);

	imem u_imem(
		.addr(pc),
		.inst(instruction)
	);

	control u_control(
		.inst(instruction),
		.alu_not_equal(alu_not_equal),
		.raddr1(raddr1),
		.raddr2(raddr2),
		.waddr(waddr),
		.gpr_we(gpr_we),
		.alu_op(alu_op),
		.immediate(immediate),
		.branch_addr(branch_addr),
		.branch_taken(branch_taken),
		.writeback_sel(writeback_sel)
	);

	gpr u_gpr(
		.clk(clk),
		.rst(rst),
		.we(gpr_we),
		.raddr1(raddr1),
		.raddr2(raddr2),
		.waddr(waddr),
		.wdata(writeback_data),
		.rdata1(rdata1),
		.rdata2(rdata2),
		.r0(r0),
		.r1(r1),
		.r2(r2),
		.r3(r3)
	);

	alu u_alu(
		.a(rdata1),
		.b(rdata2),
		.op(alu_op),
		.result(alu_result),
		.not_equal(alu_not_equal)
	);

	assign writeback_data = (writeback_sel == 2'b01)
						  ? immediate
						  : alu_result;
	assign pc_plus_one = pc + 8'd1;

	mux_pc u_mux_pc(
		.pc_plus_one(pc_plus_one),
		.branch_addr(branch_addr),
		.branch_taken(branch_taken),
		.next_pc(next_pc)
	);

endmodule
