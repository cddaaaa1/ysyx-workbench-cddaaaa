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
    wire [31:0] dmem_addr;
    wire [31:0] dmem_wdata;
    wire [3:0]  dmem_wmask;
    wire        dmem_re;
    wire        dmem_we;
    wire [31:0] dmem_rdata;

    top u_top (
        .clk(clk), 
        .rst(rst), 
        .pc(pc), 
        .inst(inst), 
        .ebreak(ebreak), 
        .misalign(misalign), 
        .ifu_raddr(ifu_raddr), 
        .ifu_rdata(ifu_rdata), 
        .dmem_addr(dmem_addr), 
        .dmem_wdata(dmem_wdata), 
        .dmem_wmask(dmem_wmask), 
        .dmem_re(dmem_re), 
        .dmem_we(dmem_we), 
        .dmem_rdata(dmem_rdata)
    );

    dpic_mem u_dpic_mem (
        .clk(clk), 
        .ifu_raddr(ifu_raddr), 
        .ifu_rdata(ifu_rdata), 
        .dmem_addr(dmem_addr), 
        .dmem_wdata(dmem_wdata), 
        .dmem_wmask(dmem_wmask), 
        .dmem_re(dmem_re), 
        .dmem_we(dmem_we), 
        .dmem_rdata(dmem_rdata)
    );


endmodule 