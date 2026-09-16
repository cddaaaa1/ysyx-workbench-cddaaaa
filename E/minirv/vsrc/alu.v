// ALU: 组合运算单元, 由 EXU 例化
// 控制码的编码必须与 idu.v / exu.v 中的定义保持一致
module alu #(DATA_WIDTH = 32) (
    input  [DATA_WIDTH-1:0]     a,
    input  [DATA_WIDTH-1:0]     b,
    input  [3:0]                op,
    output reg [DATA_WIDTH-1:0] result
);

    localparam [3:0] ALU_ADDI     = 4'd0; // a + b
    localparam [3:0] ALU_ADD = 4'd1; // a + b  (b 由 EXU 选成 rs2)
    localparam [3:0] ALU_PASS_B  = 4'd2; // b      (lui)

    always @* begin
        case (op)
            ALU_ADDI, ALU_ADD: result = a + b;
            ALU_PASS_B:        result = b;
            default:           result = {DATA_WIDTH{1'b0}};
        endcase
    end
endmodule
