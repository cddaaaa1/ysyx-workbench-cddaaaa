module sim_top (
    input clk, 
    input rst, 
    output [31:0] pc, 
    output [31:0] inst, 
    output ebreak, 
    output misalign
);  

    reg [63:0] cycle;
     always @(posedge clk) begin
        if (rst) cycle <= 64'd0;
        else     cycle <= cycle + 64'd1;
    end

    wire [31:0] ifu_raddr;
    wire [31:0] ifu_rdata;
    wire [31:0] lsu_addr;
    wire [31:0] lsu_wdata;
    wire [3:0]  lsu_wmask;
    wire        lsu_re;
    wire        lsu_wen;
    wire [31:0] lsu_rdata;
    wire        ifu_reqValid;
    wire        ifu_respValid;
    wire        lsu_reqValid;
    wire        lsu_respValid;

    top u_top (
        .clk(clk), 
        .rst(rst), 
        .pc(pc), 
        .inst(inst), 
        .ebreak(ebreak), 
        .misalign(misalign), 
        .ifu_raddr(ifu_raddr), 
        .ifu_reqValid(ifu_reqValid), 
        .ifu_respValid(ifu_respValid), 
        .ifu_rdata(ifu_rdata), 
        .lsu_addr(lsu_addr), 
        .lsu_wdata(lsu_wdata), 
        .lsu_wmask(lsu_wmask), 
        .lsu_re(lsu_re), 
        .lsu_wen(lsu_wen), 
        .lsu_reqValid(lsu_reqValid), 
        .lsu_respValid(lsu_respValid), 
        .lsu_rdata(lsu_rdata)
    );

    dpic_mem u_dpic_mem (
        .clk(clk), 
        .rst(rst), 
        .ifu_raddr(ifu_raddr), 
        .ifu_reqValid(ifu_reqValid), 
        .ifu_respValid(ifu_respValid), 
        .ifu_rdata(ifu_rdata), 
        .lsu_addr(lsu_addr), 
        .lsu_wdata(lsu_wdata), 
        .lsu_wmask(lsu_wmask), 
        .lsu_re(lsu_re), 
        .lsu_wen(lsu_wen), 
        .lsu_reqValid(lsu_reqValid), 
        .lsu_respValid(lsu_respValid), 
        .lsu_rdata(lsu_rdata)
    );


endmodule 