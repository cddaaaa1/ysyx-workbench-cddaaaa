module npc_top (
    input  clock,
	input  reset
);

`ifndef YSYXSOC
	wire        io_ifu_reqValid;
	wire [31:0] io_ifu_addr;
	wire        io_ifu_respValid;
	wire [31:0] io_ifu_rdata;
	wire        io_lsu_reqValid;
	wire [31:0] io_lsu_addr;
	wire [1:0]  io_lsu_size;
	wire        io_lsu_wen;
	wire [31:0] io_lsu_wdata;
	wire [3:0]  io_lsu_wmask;
	wire        io_lsu_respValid;
	wire [31:0] io_lsu_rdata;
`endif


ysyx_22040000 npc (
    .clock(clock), 
    .reset(reset), 
    .io_ifu_addr(io_ifu_addr),
        .io_ifu_reqValid(io_ifu_reqValid),
        .io_ifu_respValid(io_ifu_respValid),
        .io_ifu_rdata(io_ifu_rdata),
        .io_lsu_addr(io_lsu_addr),
        .io_lsu_wdata(io_lsu_wdata),
        .io_lsu_wmask(io_lsu_wmask),
        .io_lsu_size(io_lsu_size),
        .io_lsu_wen(io_lsu_wen),
        .io_lsu_reqValid(io_lsu_reqValid),
        .io_lsu_respValid(io_lsu_respValid),
        .io_lsu_rdata(io_lsu_rdata)
);


    dpic_mem u_dpic_mem (
        .clk(clock),
        .rst(reset),
        .ifu_addr(io_ifu_addr),
        .ifu_reqValid(io_ifu_reqValid),
        .ifu_respValid(io_ifu_respValid),
        .ifu_rdata(io_ifu_rdata),
        .lsu_addr(io_lsu_addr),
        .lsu_wdata(io_lsu_wdata),
        .lsu_wmask(io_lsu_wmask),
        .lsu_size(io_lsu_size),
        .lsu_wen(io_lsu_wen),
        .lsu_reqValid(io_lsu_reqValid),
        .lsu_respValid(io_lsu_respValid),
        .lsu_rdata(io_lsu_rdata)
    );

`ifdef NETLIST_SIM
	// 网表里没有 `ifndef SYNTHESIS 那套 ebreak 打印/结束逻辑, 由仿真侧在取到 ebreak 时收尾
	`include "define.vh"
	always @(posedge clock)
		if (io_ifu_respValid && io_ifu_rdata == `INST_EBREAK) $finish;
`endif

endmodule
