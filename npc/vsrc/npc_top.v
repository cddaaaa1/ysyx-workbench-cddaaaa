module npc_top (
  input clock,
  input reset
);

`ifndef YSYXSOC
  wire        io_ifu_reqValid;
  wire [`CPU_WIDTH-1:0] io_ifu_addr;
  wire        io_ifu_respValid;
  wire [`CPU_WIDTH-1:0] io_ifu_rdata;
  wire        io_lsu_reqValid;
  wire [`CPU_WIDTH-1:0] io_lsu_addr;
  wire [1:0]  io_lsu_size;
  wire        io_lsu_wen;
  wire [`CPU_WIDTH-1:0] io_lsu_wdata;
  wire [`CPU_WIDTH/8-1:0] io_lsu_wmask;
  wire        io_lsu_respValid;
  wire [`CPU_WIDTH-1:0] io_lsu_rdata;
`endif


  ysyx_26100296 npc (
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
    .clk_i(clock),
    .rst_i(reset),
    .ifu_addr_i(io_ifu_addr),
    .ifu_reqValid_i(io_ifu_reqValid),
    .ifu_respValid_o(io_ifu_respValid),
    .ifu_rdata_o(io_ifu_rdata),
    .lsu_addr_i(io_lsu_addr),
    .lsu_wdata_i(io_lsu_wdata),
    .lsu_wmask_i(io_lsu_wmask),
    .lsu_size_i(io_lsu_size),
    .lsu_wen_i(io_lsu_wen),
    .lsu_reqValid_i(io_lsu_reqValid),
    .lsu_respValid_o(io_lsu_respValid),
    .lsu_rdata_o(io_lsu_rdata)
  );

`ifdef NETLIST_SIM
`include "define.vh"
  always @(posedge clock)
    if (io_ifu_respValid && io_ifu_rdata == `INST_EBREAK) $finish;
`endif

endmodule
