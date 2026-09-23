module dpic_mem(
    input clk, 
    input [31:0] ifu_raddr, 
    input ifu_reqValid, 
    output reg [31:0] ifu_rdata,
    output reg ifu_respValid,
    input [31:0] lsu_addr, 
    input [31:0] lsu_wdata, 
    input [3:0] lsu_wmask,
    input lsu_re, 
    input lsu_wen, 
    input lsu_reqValid, 
    output reg [31:0] lsu_rdata,
    output reg lsu_respValid
);
    import "DPI-C" function int  pmem_read (input int raddr);
    import "DPI-C" function void pmem_write(input int waddr, input int wdata, input byte wmask);

    // 取指:
    always @(posedge clk) begin
        ifu_rdata     <= ifu_reqValid ? pmem_read(ifu_raddr) : ifu_rdata; // 注意: 不清零
        ifu_respValid <= ifu_reqValid;
    end

     // 访存
    always @(posedge clk) begin
        lsu_rdata <= (lsu_reqValid && !lsu_wen) ? pmem_read(lsu_addr) : 32'b0;
        if (lsu_reqValid && lsu_wen) begin
            pmem_write(lsu_addr, lsu_wdata, {4'h0, lsu_wmask});
        end
        lsu_respValid <= lsu_reqValid;
    end
endmodule