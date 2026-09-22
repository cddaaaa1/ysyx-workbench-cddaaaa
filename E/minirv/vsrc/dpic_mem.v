module dpic_mem(
    input clk, 
    input [31:0] ifu_raddr, output reg [31:0] ifu_rdata,
    input [31:0] lsu_addr, input [31:0] lsu_wdata, input [3:0] lsu_wmask,
    input lsu_re, input lsu_wen, output reg [31:0] lsu_rdata
);
    import "DPI-C" function int  pmem_read (input int raddr);
    import "DPI-C" function void pmem_write(input int waddr, input int wdata, input byte wmask);

    // 取指: 按 SimpleBus 协议, 收到读请求后延迟一拍才返回数据
    //       (发请求那拍地址被采样, 数据在下一拍才出现在 ifu_rdata 上)
    always @(posedge clk) begin
        ifu_rdata <= pmem_read(ifu_raddr);
    end

    // 数据访问: 和取指一致 —— 读延迟一拍, 写在发出请求的那一拍完成。
    // lsu_addr 已由 LSU 门控(非访存周期给 0): Verilator 会把 DPI-C 调用从三元
    // 表达式里提出来无条件求值, 所以门控必须做在地址上, 不能只做在条件上。
    // lsu_wmask 只有 4 位, 而 pmem_write 的 wmask 是 byte, 需要显式补零扩展
    // (否则 Verilator 报 WIDTHEXPAND, 而 warning 默认是致命的)。
    always @(posedge clk) begin
        lsu_rdata <= (!lsu_wen) ? pmem_read(lsu_addr) : 32'b0;
        if (lsu_wen) begin
            pmem_write(lsu_addr, lsu_wdata, {4'h0, lsu_wmask});
        end
    end
endmodule