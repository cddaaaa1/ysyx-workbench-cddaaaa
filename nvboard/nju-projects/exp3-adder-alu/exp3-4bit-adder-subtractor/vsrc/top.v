module top(
  input  [3:0] a,
  input  [3:0] b,
  input        sub,
  output [3:0] s,
  output       of,
  output       cf,
  output       zf
);
  alu4 u_alu4 (
    .a   (a),
    .b   (b),
    .sub (sub),
    .s   (s),
    .of  (of),
    .cf  (cf),
    .zf  (zf)
  );
endmodule
