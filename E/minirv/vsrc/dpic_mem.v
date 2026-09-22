module dpic_mem(
    input clk, 
    input [31:0] ifu_raddr, output reg [31:0] ifu_rdata,
    input [31:0] dmem_addr, input [31:0] dmem_wdata, input [3:0] dmem_wmask,
    input dmem_re, input dmem_we, output [31:0] dmem_rdata
);
    import "DPI-C" function int  pmem_read (input int raddr);
    import "DPI-C" function void pmem_write(input int waddr, input int wdata, input byte wmask);

    // 取指部分按 SimpleBus 协议: 收到读请求后延迟一拍才返回数据
    always @(posedge clk) begin
        ifu_rdata <= pmem_read(ifu_raddr);
    end

    // 数据访问部分暂不改动, 仍是 0 延迟的组合读
    assign dmem_rdata = dmem_re ? pmem_read(dmem_addr) : 32'h0;
    always @(posedge clk) if (dmem_we) pmem_write(dmem_addr, dmem_wdata, {4'h0, dmem_wmask});
endmodule