module top(
  input  btn,
  output [7:0] seg0,
  output [7:0] seg1
);
  reg [7:0] lfsr = 8'h01;
  wire [6:0] low_hex;
  wire [6:0] high_hex;

  // x8 = x4 ^ x3 ^ x2 ^ x0; shift right on every button edge.
  always @(posedge btn) begin
    if (lfsr == 8'h00)
      lfsr <= 8'h01;
    else
      lfsr <= {
        lfsr[4] ^ lfsr[3] ^ lfsr[2] ^ lfsr[0],
        lfsr[7:1]
      };
  end

  hex7seg u_low_hex (
    .b (lfsr[3:0]),
    .h (low_hex)
  );

  hex7seg u_high_hex (
    .b (lfsr[7:4]),
    .h (high_hex)
  );

  // hex7seg output already uses NVBoard order {A, B, C, D, E, F, G, DP}.
  assign seg0 = {low_hex, 1'b1};
  assign seg1 = {high_hex, 1'b1};
endmodule