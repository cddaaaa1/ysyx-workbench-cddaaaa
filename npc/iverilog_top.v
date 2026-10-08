`timescale 1ns/1ps
module iverilog_top;
	reg clock = 0;
	reg reset = 1;
	reg [8*256-1:0] img;

	ysyx_22040000 dut(.clock(clock), .reset(reset));

	always #1 clock = ~clock;

	initial begin
		// 镜像路径由 +img=<path> 传入, 在第一条取指之前加载
		if (!$value$plusargs("img=%s", img)) begin
			$display("[TB] 需要 +img=<path>");
			$finish;
		end
		$pmem_load(img);
`ifdef DUMP
		$dumpfile("dump.vcd");
		$dumpvars(0, iverilog_top);
`endif
		#10 reset = 0;
	end
endmodule
