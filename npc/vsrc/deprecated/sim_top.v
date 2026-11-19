`include "define.vh"

module sim_top (
  input                   clk_i,
  input                   rst_i,
  output [`CPU_WIDTH-1:0] pc_o,
  output [`CPU_WIDTH-1:0] inst_o,
  output                  ebreak_o,
  output                  misalign_o
);

  reg [63:0] cycle;
  always @(posedge clk_i) begin
    if (rst_i) cycle <= 64'd0;
    else     cycle <= cycle + 64'd1;
  end

  wire [`CPU_WIDTH-1:0] ifu_addr;
  wire [`CPU_WIDTH-1:0] ifu_rdata;
  wire [`CPU_WIDTH-1:0] lsu_addr;
  wire [`CPU_WIDTH-1:0] lsu_wdata;
  wire [`CPU_WIDTH/8-1:0] lsu_wmask;
  wire [1:0]  lsu_size;
  wire        lsu_wen;
  wire [`CPU_WIDTH-1:0] lsu_rdata;
  wire        ifu_reqValid;
  wire        ifu_respValid;
  wire        lsu_reqValid;
  wire        lsu_respValid;

  ysyx_26100296 u_cpu (
    .clock(clk_i),
    .reset(rst_i),
    .io_ifu_addr(ifu_addr),
    .io_ifu_reqValid(ifu_reqValid),
    .io_ifu_respValid(ifu_respValid),
    .io_ifu_rdata(ifu_rdata),
    .io_lsu_addr(lsu_addr),
    .io_lsu_wdata(lsu_wdata),
    .io_lsu_wmask(lsu_wmask),
    .io_lsu_size(lsu_size),
    .io_lsu_wen(lsu_wen),
    .io_lsu_reqValid(lsu_reqValid),
    .io_lsu_respValid(lsu_respValid),
    .io_lsu_rdata(lsu_rdata)
  );

  assign pc_o       = u_cpu.pc;
  assign inst_o     = u_cpu.inst;
  assign ebreak_o   = u_cpu.ebreak;
  assign misalign_o = u_cpu.misalign;

  dpic_mem u_dpic_mem (
    .clk_i(clk_i),
    .rst_i(rst_i),
    .ifu_addr_i(ifu_addr),
    .ifu_reqValid_i(ifu_reqValid),
    .ifu_respValid_o(ifu_respValid),
    .ifu_rdata_o(ifu_rdata),
    .lsu_addr_i(lsu_addr),
    .lsu_wdata_i(lsu_wdata),
    .lsu_wmask_i(lsu_wmask),
    .lsu_size_i(lsu_size),
    .lsu_wen_i(lsu_wen),
    .lsu_reqValid_i(lsu_reqValid),
    .lsu_respValid_o(lsu_respValid),
    .lsu_rdata_o(lsu_rdata)
  );


endmodule
