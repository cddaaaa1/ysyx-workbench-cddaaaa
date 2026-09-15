module imem #(
	parameter [1023:0] INIT_FILE = "program/sum.hex"
)(
	input  [7:0] addr,
	output [7:0] inst
);

	reg [7:0] mem [0:255];

	initial begin
		if (INIT_FILE != "")
			$readmemh(INIT_FILE, mem);
	end

	assign inst = mem[addr];

endmodule
