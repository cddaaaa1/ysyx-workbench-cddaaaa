// 4-bit two's complement adder/subtractor.
// sub = 0: s = a + b
// sub = 1: s = a - b
module alu4(
  input  [3:0] a,
  input  [3:0] b,
  input        sub,
  output [3:0] s,  // 结果（4 位补码）
  output       of,  // 有符号溢出标志
  output       cf,  // 无符号进位标志
  output       zf   // 零标志
);
  wire [3:0] b_in;
  wire       cin;
  wire [4:0] sum;

  // Subtraction is implemented as addition with a negated operand:
  // a - b = a + (~b) + 1
  assign b_in = sub ? ~b : b;
  assign cin  = sub;

  // 5-bit sum keeps the carry out from the most significant bit.
  assign sum = {1'b0, a} + {1'b0, b_in} + {4'b0, cin};

  assign s  = sum[3:0];
  assign cf = sum[4];

  // Signed overflow: adding operands with equal signs must not flip the
  // sign of the result. This also covers subtraction via b_in = ~b.
  assign of = (a[3] == b_in[3]) && (s[3] != a[3]);

  assign zf = (s == 4'b0000);
endmodule
