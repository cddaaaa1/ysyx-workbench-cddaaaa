module control(
	input  [7:0] inst,
	input        alu_not_equal,
	output reg [1:0] raddr1,
	output reg [1:0] raddr2,
	output reg [1:0] waddr,
	output reg       gpr_we,
	output reg [1:0] alu_op,
	output reg [7:0] immediate,
	output reg [7:0] branch_addr,
	output reg       branch_taken,
	output reg [1:0] writeback_sel
);

	localparam [1:0] ALU_ADD = 2'b00;
	localparam [1:0] ALU_CMP = 2'b01;
	localparam [1:0] WB_ALU  = 2'b00;
	localparam [1:0] WB_IMM  = 2'b01;

	always @* begin
		raddr1 = 2'b00;
		raddr2 = 2'b00;
		waddr = inst[5:4];
		gpr_we = 1'b0;
		alu_op = ALU_ADD;
		immediate = 8'h00;
		branch_addr = 8'h00;
		branch_taken = 1'b0;
		writeback_sel = WB_ALU;

		case (inst[7:6])
			2'b00: begin
				// add: R[rd] = R[rs1] + R[rs2]
				raddr1 = inst[3:2];
				raddr2 = inst[1:0];
				gpr_we = 1'b1;
			end
			2'b10: begin
				// li: R[rd] = zero_extend(imm)
				immediate = {4'b0000, inst[3:0]};
				gpr_we = 1'b1;
				writeback_sel = WB_IMM;
			end
			2'b11: begin
				// bner0: if (R[0] != R[rs2]) PC = addr
				raddr1 = 2'b00;
				raddr2 = inst[2:1];
				alu_op = ALU_CMP;
				branch_addr = {5'b00000, inst[5:3]};
				branch_taken = alu_not_equal;
			end
			default: begin
			end
		endcase
	end

endmodule
