`include "define.vh"
module ysyx_26100296_pc_reg (
  input        clk_i,
  input        rst_i,
  input        we_i,
  input  [`CPU_WIDTH-1:0] next_pc_i,
  output reg [`CPU_WIDTH-1:0] pc_o
);

  always @(posedge clk_i) begin
    if (rst_i)
      pc_o <= `PC_RESET;
    else if (we_i)
      pc_o <= next_pc_i;
  end
endmodule
