module alu(
    input  [7:0] a,
    input  [7:0] b,
    input  [1:0] op,
    output reg [7:0] result,
    output           not_equal
);

    localparam [1:0] ALU_ADD = 2'b00;
    localparam [1:0] ALU_CMP = 2'b01;

    always @* begin
        case (op)
            ALU_ADD: result = a + b;
            ALU_CMP: result = 8'h00;
            default: result = 8'h00;
        endcase
    end

    assign not_equal = (a != b);

endmodule