// EXU (EXecution Unit): 根据控制信号控制 ALU 进行计算, 并计算跳转目标
// 内部包含 ALU
module exu(
	input  [31:0] rdata1,      // rs1 的值, 来自 gpr 读端口
	input  [31:0] rdata2,      // rs2 的值, 来自 gpr 读端口
	input  [31:0] imm,         // 立即数, 来自 IDU
	input  [3:0]  alu_op,      // ALU 控制码, 来自 IDU
	input         is_jalr,     // 是否 jalr
	output [31:0] alu_result,  // 运算结果 (访存指令时即访存地址)
	output        jump,        // 是否发生跳转
	output [31:0] jump_target  // 跳转目标地址
);

	// EXU 只需要区分"操作数 b 用 rs2 还是用立即数"
        // (完整的 ALU 控制码编码见 idu.v 与 alu.v)
    localparam [3:0] ALU_ADD = 4'd1; // a + b, b 取 rs2 (add)

	wire [31:0] alu_b = (alu_op == ALU_ADD) ? rdata2 : imm;

	alu u_alu(
		.a(rdata1),
		.b(alu_b),
		.op(alu_op),
		.result(alu_result)
	);

	// jalr 的目标地址就是 (rs1 + imm), 因此直接复用 ALU 的结果, 只把最低位清零
	// (RISC-V 规定 jalr 目标的最低比特恒为 0)
	assign jump        = is_jalr;
	assign jump_target = alu_result & ~32'h1;

endmodule

