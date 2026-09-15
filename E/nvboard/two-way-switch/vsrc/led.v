module led(
  input clk,
  input rst,
  input [4:0] btn,
  input [7:0] sw,
  input sw_2way_out,
  output [15:0] ledr
);
  reg [31:0] count;
  reg [7:0] led;
  // always @(posedge clk) begin
  //   if (rst) begin led <= 1; count <= 0; end
  //   else begin
  //     if (count == 0) led <= {led[6:0], led[7]};
  //     count <= (count >= 5000000 ? 32'b0 : count + 1);
  //   end
  // end

  // assign ledr = {sw_2way_out, led[7:5], led[4:0] ^ btn, sw[6:0]}; 
  assign ledr = {15'b0, sw_2way_out};
endmodule
