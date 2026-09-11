module bcd7seg(
  input  [2:0] b,
  output reg [6:0] h
);
  // h is {g, f, e, d, c, b, a}; segments are active-low.
  always @(*) begin
    case (b)
      3'b000: h = 7'b1000000;
      3'b001: h = 7'b1111001;
      3'b010: h = 7'b0100100;
      3'b011: h = 7'b0110000;
      3'b100: h = 7'b0011001;
      3'b101: h = 7'b0010010;
      3'b110: h = 7'b0000010;
      3'b111: h = 7'b1111000;
      default: h = 7'b1111111;
    endcase
  end
endmodule
