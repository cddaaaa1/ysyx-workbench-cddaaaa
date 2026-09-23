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

    wire [31:0] ifu_addr;
    wire [31:0] ifu_rdata;
    wire [31:0] lsu_addr;
    wire [31:0] lsu_wdata;
    wire [3:0]  lsu_wmask;
    wire [1:0]  lsu_size;
    wire        lsu_wen;
    wire [31:0] lsu_rdata;
    wire        ifu_reqValid;
    wire        ifu_respValid;
    wire        lsu_reqValid;
    wire        lsu_respValid;

    // CPU 顶层端口严格按 cpu-interface.md 命名, 这里一一对应地连出来
    top u_top (
        .clock(clk), 
        .reset(rst), 
        .io_ifu_addr(ifu_addr), 
        .io_ifu_reqValid(ifu_reqValid), 
        .io_ifu_respValid(ifu_respValid), 
        .io_ifu_rdata(ifu_rdata), 
        .io_lsu_addr(lsu_addr), 
        .io_lsu_wdata(lsu_wdata), 
        .io_lsu_wmask(lsu_wmask), 
        .io_lsu_size(lsu_size), 
        .io_lsu_wen(lsu_wen), 
        .io_lsu_reqValid(lsu_reqValid), 
        .io_lsu_respValid(lsu_respValid), 
        .io_lsu_rdata(lsu_rdata)
    );

    // 观察信号: 用层次引用从 CPU 内部读, 这样 CPU 端口不必为仿真多长几个出来
    assign pc       = u_top.pc;
    assign inst     = u_top.inst;
    assign ebreak   = u_top.ebreak;
    assign misalign = u_top.misalign;

    dpic_mem u_dpic_mem (
        .clk(clk), 
        .rst(rst), 
        .ifu_addr(ifu_addr), 
        .ifu_reqValid(ifu_reqValid), 
        .ifu_respValid(ifu_respValid), 
        .ifu_rdata(ifu_rdata), 
        .lsu_addr(lsu_addr), 
        .lsu_wdata(lsu_wdata), 
        .lsu_wmask(lsu_wmask), 
        .lsu_size(lsu_size), 
        .lsu_wen(lsu_wen), 
        .lsu_reqValid(lsu_reqValid), 
        .lsu_respValid(lsu_respValid), 
        .lsu_rdata(lsu_rdata)
    );


endmodule 