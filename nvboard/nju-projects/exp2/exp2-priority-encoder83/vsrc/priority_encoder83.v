module priority_encoder83(
  input  [7:0] x,
  input        en,
  output reg [2:0] code,
  output reg       valid
);
  integer i;

  always @(*) begin
    code = 3'b000;
    valid = 1'b0;

    if (en) begin
      for (i = 7; i >= 0; i = i - 1) begin
        if (x[i] && !valid) begin
          code = i[2:0];
          valid = 1'b1;
        end
      end
    end
  end
endmodule

