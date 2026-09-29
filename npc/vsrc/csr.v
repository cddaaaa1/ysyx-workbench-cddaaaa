`include "define.vh"
module ysyx_22040000_csr(
	input         clk,
	input         rst,
	input  [11:0] addr,
	output [31:0] rdata
);

	wire [31:0] mvendorid;
	wire [31:0] marchid;
    wire [63:0] mcycle;

	stdreg #(
		.WIDTH    (32),
		.RESET_VAL(`MVENDORID_VAL)
	) reg_mvendorid (
		.i_clk  (clk),
		.i_rst  (rst),
		.i_wen  (1'b0),
		.i_din  (32'b0),
		.o_dout (mvendorid)
	);

	stdreg #(
		.WIDTH    (32),
		.RESET_VAL(`MARCHID_VAL)
	) reg_marchid (
		.i_clk  (clk),
		.i_rst  (rst),
		.i_wen  (1'b0),
		.i_din  (32'b0),
		.o_dout (marchid)
	);

    stdreg #(
		.WIDTH    (64),
		.RESET_VAL(64'b0)
	) reg_mcycle (
		.i_clk  (clk),
		.i_rst  (rst),
		.i_wen  (1'b1),
		.i_din  (mcycle + 64'd1),
		.o_dout (mcycle)
	);

	assign rdata = (addr == `CSR_MVENDORID) ? mvendorid     :
	               (addr == `CSR_MARCHID)   ? marchid       :
	               (addr == `CSR_MCYCLE)    ? mcycle[31:0]  :
	               (addr == `CSR_MCYCLEH)   ? mcycle[63:32] :
	                                          32'h0;

endmodule
