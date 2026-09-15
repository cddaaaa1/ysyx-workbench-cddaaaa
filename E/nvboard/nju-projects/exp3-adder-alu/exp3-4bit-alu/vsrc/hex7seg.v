// Hexadecimal seven-segment decoder (0-F), active-low segments.
// h = {g, f, e, d, c, b, a}.
module hex7seg(
  input  [3:0] b,
  output reg [6:0] h
);
  always @(*) begin
    case (b)
      4'h0: h = 7'b1000000;
      4'h1: h = 7'b1111001;
      4'h2: h = 7'b0100100;
      4'h3: h = 7'b0110000;
      4'h4: h = 7'b0011001;
      4'h5: h = 7'b0010010;
      4'h6: h = 7'b0000010;
      4'h7: h = 7'b1111000;
      4'h8: h = 7'b0000000;
      4'h9: h = 7'b0010000;
      4'hA: h = 7'b0001000;
      4'hB: h = 7'b0000011;
      4'hC: h = 7'b1000110;
      4'hD: h = 7'b0100001;
      4'hE: h = 7'b0000110;
      4'hF: h = 7'b0001110;
      default: h = 7'b1111111;
    endcase
  end
endmodule
