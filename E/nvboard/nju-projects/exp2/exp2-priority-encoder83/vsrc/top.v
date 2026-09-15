module top(
  input  [7:0] sw,
  output [15:0] ledr,
  output [7:0] seg0
);
  wire [2:0] code;
  wire       valid;
  wire [6:0] hex0;

  // Hardware uses SW7-SW0; the encoder is permanently enabled.
  priority_encoder83 u_encoder (
    .x     (sw),
    .en    (1'b1),
    .code  (code),
    .valid (valid)
  );

  bcd7seg u_bcd7seg (
    .b (code),
    .h (hex0)
  );

  // LED4 is valid, LED2:0 is the encoded index.
  assign ledr = {11'b0, valid, 1'b0, code};
  // bcd7seg.h is {G, F, E, D, C, B, A}; seg0 is {A, B, C, D, E, F, G, DP}.
  assign seg0 = {hex0[0], hex0[1], hex0[2], hex0[3],
                 hex0[4], hex0[5], hex0[6], 1'b1};
endmodule
