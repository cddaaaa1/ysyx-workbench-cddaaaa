// PC 寄存器: AM 的 minirv-npc 运行时环境约定程序从 0x80000000 开始取指,
// 只有一条指令执行结束(we 有效)时才更新为 next_pc, 取指等待期间保持不变
module ysyx_22040000_pc_reg (
	input        clk,
	input        rst,
	input        we,         // 写使能: 无有效指令时 PC 不变
	input  [31:0] next_pc,
	output reg [31:0] pc
);

	always @(posedge clk) begin
		if (rst)
			//pc <= 32'h0;
			pc <= 32'h30000000;
		else if (we)
			pc <= next_pc;
	end
endmodule
