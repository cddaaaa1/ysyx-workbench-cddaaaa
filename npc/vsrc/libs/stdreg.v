// 通用寄存器: 同步高电平复位, i_wen 有效时在时钟上升沿载入 i_din
module stdreg #(
	parameter WIDTH     = 32,
	parameter RESET_VAL = 0
)(
	input                i_clk,
	input                i_rst,
	input                i_wen,
	input  [WIDTH-1:0]   i_din,
	output reg [WIDTH-1:0] o_dout
);

	always @(posedge i_clk) begin
		if (i_rst)     o_dout <= RESET_VAL;
		else if (i_wen)   o_dout <= i_din;
	end

endmodule
