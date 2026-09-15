module mux_pc(
	input  [7:0] pc_plus_one,
	input  [7:0] branch_addr,
	input        branch_taken,
	output [7:0] next_pc
);

	assign next_pc = branch_taken ? branch_addr : pc_plus_one;

endmodule
