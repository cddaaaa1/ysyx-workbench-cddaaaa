module sim_top (
    input clk, 
    input rst, 
    output [31:0] pc, 
    output [31:0] inst, 
    output ebreak, 
    output misalign
);  

    wire [31:0] ifu_raddr;
    wire [31:0] ifu_rdata;
    wire [31:0] lsu_addr;
    wire [31:0] lsu_wdata;
    wire [3:0]  lsu_wmask;
    wire        lsu_re;
    wire        lsu_wen;
    wire [31:0] lsu_rdata;

    top u_top (
        .clk(clk), 
        .rst(rst), 
        .pc(pc), 
        .inst(inst), 
        .ebreak(ebreak), 
        .misalign(misalign), 
        .ifu_raddr(ifu_raddr), 
        .ifu_rdata(ifu_rdata), 
        .lsu_addr(lsu_addr), 
        .lsu_wdata(lsu_wdata), 
        .lsu_wmask(lsu_wmask), 
        .lsu_re(lsu_re), 
        .lsu_wen(lsu_wen), 
        .lsu_rdata(lsu_rdata)
    );

    dpic_mem u_dpic_mem (
        .clk(clk), 
        .ifu_raddr(ifu_raddr), 
        .ifu_rdata(ifu_rdata), 
        .lsu_addr(lsu_addr), 
        .lsu_wdata(lsu_wdata), 
        .lsu_wmask(lsu_wmask), 
        .lsu_re(lsu_re), 
        .lsu_wen(lsu_wen), 
        .lsu_rdata(lsu_rdata)
    );


endmodule 