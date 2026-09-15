module top(
  input  [1:0] x,
  input        en,
  output [3:0] y
);
  decode24 u_decode24 (
    .x  (x),
    .en (en),
    .y  (y)
  );
endmodule