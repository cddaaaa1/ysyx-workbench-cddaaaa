module pc_reg (
	input        clk,
	input        rst,
	input  [7:0] next_pc,
	output reg [7:0] pc
);

	always @(posedge clk) begin
		if (rst)
			pc <= 8'h00;
		else
			pc <= next_pc;
	end
endmodule

