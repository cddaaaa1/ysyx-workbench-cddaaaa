module top(
  input  [7:0] sw,     // SW[3:0] = A, SW[7:4] = B
  input  [2:0] btn,    // 3 buttons select the ALU operation
  output [15:0] ledr,  // LD3..LD0 = result, LD4 = of, LD5 = cf, LD6 = zf
  output [7:0] seg0    // seven-segment display of the result
);
  wire [3:0] out;
  wire       of;
  wire       cf;
  wire       zf;
  wire [6:0] hex0;

  alu u_alu (
    .a    (sw[3:0]),
    .b    (sw[7:4]),
    .ctrl (btn),
    .out  (out),
    .of   (of),
    .cf   (cf),
    .zf   (zf)
  );

  hex7seg u_hex7seg (
    .b (out),
    .h (hex0)
  );

  assign ledr = {9'b0, zf, cf, of, out};

  // NVBoard segment order is {A, B, C, D, E, F, G, DP};
  // hex7seg.h is {g, f, e, d, c, b, a}; decimal point stays off.
  assign seg0 = {hex0[0], hex0[1], hex0[2], hex0[3],
                 hex0[4], hex0[5], hex0[6], 1'b1};
endmodule
