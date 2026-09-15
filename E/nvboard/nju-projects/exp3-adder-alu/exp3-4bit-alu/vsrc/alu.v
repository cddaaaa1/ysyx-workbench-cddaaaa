// 4-bit two's complement ALU.
//
// ctrl | operation | result
// -----+-----------+------------------------------
// 000  | add       | a + b
// 001  | sub       | a - b
// 010  | not       | ~a
// 011  | and       | a & b
// 100  | or        | a | b
// 101  | xor       | a ^ b
// 110  | signed lt | (a < b) ? 1 : 0
// 111  | equal     | (a == b) ? 1 : 0
//
// of and cf are meaningful only for add/sub; they are 0 for logic ops.
module alu(
  input  [3:0] a,
  input  [3:0] b,
  input  [2:0] ctrl,
  output reg [3:0] out,  // 结果
  output reg       of,   // 有符号溢出标志（仅加减有效）
  output reg       cf,   // 无符号进位标志（仅加减有效）
  output reg       zf    // 零标志
);
  wire [3:0] b_in;
  wire       cin;
  wire [4:0] sum;
  wire       add_of;
  wire       sub_of;
  wire       signed_lt;

  // Subtraction reuses the adder: a - b = a + (~b) + 1.
  assign b_in = (ctrl == 3'b001) ? ~b : b;
  assign cin  = (ctrl == 3'b001);

  // 5-bit sum keeps the carry out from the most significant bit.
  assign sum = {1'b0, a} + {1'b0, b_in} + {4'b0, cin};

  // Overflow: operands with equal signs must not flip the sign of the result.
  assign add_of = (a[3] == b[3]) && (sum[3] != a[3]);
  assign sub_of = (a[3] == b_in[3]) && (sum[3] != a[3]);

  // Signed comparison: when signs differ the negative one is smaller;
  // otherwise compare the remaining bits as unsigned.
  assign signed_lt = (a[3] != b[3]) ? a[3] : (a[2:0] < b[2:0]);

  always @(*) begin
    case (ctrl)
      3'b000: begin out = sum[3:0]; of = add_of; cf = sum[4]; end
      3'b001: begin out = sum[3:0]; of = sub_of; cf = sum[4]; end
      3'b010: begin out = ~a;       of = 1'b0;   cf = 1'b0;   end
      3'b011: begin out = a & b;    of = 1'b0;   cf = 1'b0;   end
      3'b100: begin out = a | b;    of = 1'b0;   cf = 1'b0;   end
      3'b101: begin out = a ^ b;    of = 1'b0;   cf = 1'b0;   end
      3'b110: begin out = signed_lt ? 4'd1 : 4'd0; of = 1'b0; cf = 1'b0; end
      3'b111: begin out = (a == b) ? 4'd1 : 4'd0; of = 1'b0; cf = 1'b0; end
      default: begin out = 4'b0; of = 1'b0; cf = 1'b0; end
    endcase
    zf = (out == 4'b0000);
  end
endmodule
