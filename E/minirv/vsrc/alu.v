`include "define.vh"
// ALU: 组合运算单元, 由 EXU 例化
// 控制码的定义见 vsrc/include/define.vh
module alu #(DATA_WIDTH = 32) (
    input  [DATA_WIDTH-1:0]     a,
    input  [DATA_WIDTH-1:0]     b,
    input  [3:0]                op,
    output reg [DATA_WIDTH-1:0] result
);

    always @* begin
        case (op)
            `ALU_ADDI, `ALU_ADD: result = a + b;
            `ALU_PASS_B:        result = b;
            default:           result = {DATA_WIDTH{1'b0}};
        endcase
    end
endmodule
