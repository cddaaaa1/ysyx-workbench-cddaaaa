`include "define.vh"
module ysyx_26100296_wbu(
  input  [`CPU_WIDTH-1:0] pc_plus4_i,
  input  [1:0]            wb_sel_i,
  input  [`CPU_WIDTH-1:0] alu_result_i,
  input  [`CPU_WIDTH-1:0] mem_rdata_i,
  input  [`CPU_WIDTH-1:0] csr_rdata_i,
  output [`CPU_WIDTH-1:0] wb_data_o
);

  MuxKey #(4, 2, `CPU_WIDTH) u_wb_data (
    .out(wb_data_o), .key(wb_sel_i),
    .lut({`WB_ALU, alu_result_i,
          `WB_MEM, mem_rdata_i,
          `WB_PC4, pc_plus4_i,
          `WB_CSR, csr_rdata_i})
  );

endmodule
