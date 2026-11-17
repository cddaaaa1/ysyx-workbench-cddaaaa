`include "define.vh"
module ysyx_26100296_wbu(
  input  [`CPU_WIDTH-1:0] pc_i,
  input  [1:0]  wb_sel_i,
  input  [`CPU_WIDTH-1:0] alu_result_i,
  input  [`CPU_WIDTH-1:0] mem_rdata_i,
  input  [`CPU_WIDTH-1:0] csr_rdata_i,
  input         redirect_i,
  input  [`CPU_WIDTH-1:0] redirect_target_i,
  output reg [`CPU_WIDTH-1:0] wb_data_o,
  output reg [`CPU_WIDTH-1:0] next_pc_o
);

  wire [`CPU_WIDTH-1:0] pc_plus4 = pc_i + `CPU_WIDTH'd4;

  always @* begin
    case (wb_sel_i)
      `WB_ALU: wb_data_o = alu_result_i;
      `WB_MEM: wb_data_o = mem_rdata_i;
      `WB_PC4: wb_data_o = pc_plus4;
      `WB_CSR: wb_data_o = csr_rdata_i;
      default: wb_data_o = {`CPU_WIDTH{1'b0}};
    endcase
  end

  assign next_pc_o = redirect_i ? redirect_target_i : pc_plus4;
endmodule
