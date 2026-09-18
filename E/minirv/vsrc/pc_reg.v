// PC 寄存器: AM 的 minirv-npc 运行时环境约定程序从 0x80000000 开始取指,
// 每个时钟沿更新为 next_pc
module pc_reg (
	input        clk,
	input        rst,
	input  [31:0] next_pc,
	output reg [31:0] pc
);

	always @(posedge clk) begin
		if (rst)
			//pc <= 32'h0;
			pc <= 32'h80000000;
		else
			pc <= next_pc;
	end
endmodule
