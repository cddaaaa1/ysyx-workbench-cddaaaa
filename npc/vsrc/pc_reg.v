`include "define.vh"
module ysyx_22040000_pc_reg (
	input        clk,
	input        rst,
	input        we,
	input  [31:0] next_pc,
	output reg [31:0] pc
);

	always @(posedge clk) begin
		if (rst)
			pc <= `PC_RESET;
		else if (we)
			pc <= next_pc;
	end
endmodule
