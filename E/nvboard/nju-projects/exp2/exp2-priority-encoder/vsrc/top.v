module top(
  input  [3:0] x,
  input        en,
  output [1:0] y
);
  encode42 u_encode42 (
    .x  (x),
    .en (en),
    .y  (y)
  );
endmodule