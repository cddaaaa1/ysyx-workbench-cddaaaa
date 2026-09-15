module top(
  input  [23:0] cpudbgdata,
  output [6:0] HEX5,
  output [6:0] HEX4,
  output [6:0] HEX3,
  output [6:0] HEX2,
  output [6:0] HEX1,
  output [6:0] HEX0
);
  // Output debug information to six seven-segment displays.
  bcd7seg seg5(cpudbgdata[23:20], HEX5);
  bcd7seg seg4(cpudbgdata[19:16], HEX4);
  bcd7seg seg3(cpudbgdata[15:12], HEX3);
  bcd7seg seg2(cpudbgdata[11:8],  HEX2);
  bcd7seg seg1(cpudbgdata[7:4],   HEX1);
  bcd7seg seg0(cpudbgdata[3:0],   HEX0);
endmodule
