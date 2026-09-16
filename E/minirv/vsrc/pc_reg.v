// PC 寄存器: minirv 规定复位后 PC = 0, 每个时钟沿更新为 next_pc
module pc_reg (
	input        clk,
	input        rst,
	input  [31:0] next_pc,
	output reg [31:0] pc
);

	always @(posedge clk) begin
		if (rst)
			pc <= 32'h0;
		else
			pc <= next_pc;
	end
endmodule
