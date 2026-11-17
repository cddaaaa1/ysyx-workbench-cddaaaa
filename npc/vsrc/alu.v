`include "define.vh"

module ysyx_26100296_alu #(DATA_WIDTH = `CPU_WIDTH) (
  input  [DATA_WIDTH-1:0]     a_i,
  input  [DATA_WIDTH-1:0]     b_i,
  input  [3:0]                op_i,
  output reg [DATA_WIDTH-1:0] result_o,
  output reg                  cmp_result_o
);

  always @* begin
    result_o = {DATA_WIDTH{1'b0}};
    cmp_result_o = 1'b0;

    case (op_i)
      `ALU_ADD: result_o = a_i + b_i;
      `ALU_SUB: result_o = a_i - b_i;

      `ALU_AND: result_o = a_i & b_i;
      `ALU_OR:  result_o = a_i | b_i;
      `ALU_XOR: result_o = a_i ^ b_i;

      `ALU_SLL: result_o = a_i << b_i[4:0];
      `ALU_SRL: result_o = a_i >> b_i[4:0];
      `ALU_SRA: result_o = $signed(a_i) >>> b_i[4:0];

      `ALU_PASS_B: result_o = b_i;

      `ALU_SLT: begin
        cmp_result_o = ($signed(a_i) < $signed(b_i));
        result_o = {{(DATA_WIDTH-1){1'b0}}, cmp_result_o};
      end
      `ALU_SLTU: begin
        cmp_result_o = (a_i < b_i);
        result_o = {{(DATA_WIDTH-1){1'b0}}, cmp_result_o};
      end

      `ALU_EQ: cmp_result_o = (a_i == b_i);
      `ALU_NE: cmp_result_o = (a_i != b_i);
      `ALU_GE: cmp_result_o = ($signed(a_i) >= $signed(b_i));
      `ALU_GEU:cmp_result_o = (a_i >= b_i);
      default: begin end
    endcase
  end
endmodule
