module gpr(
    input clk,
    input rst,
    input we,
    input [1:0] raddr1,
    input [1:0] raddr2,
    input [1:0] waddr,
    input [7:0] wdata,
    output [7:0] rdata1,
    output [7:0] rdata2,
    output [7:0] r0,
    output [7:0] r1,
    output [7:0] r2,
    output [7:0] r3
);

    reg [7:0] regs [0:3];
    integer i;

    always @(posedge clk) begin
        if (rst) begin
            for (i = 0; i < 4; i = i + 1)
                regs[i] <= 8'h00;
        end
        else if (we) begin
            regs[waddr] <= wdata;
        end
    end

    assign rdata1 = regs[raddr1];
    assign rdata2 = regs[raddr2];
    assign r0 = regs[0];
    assign r1 = regs[1];
    assign r2 = regs[2];
    assign r3 = regs[3];

endmodule