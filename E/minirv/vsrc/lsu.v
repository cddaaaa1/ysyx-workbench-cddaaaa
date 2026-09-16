// LSU (Load-Store Unit): 根据控制信号访问存储器
// 存储器由 C++ 实现, 通过 DPI-C 的 pmem_read / pmem_write 访问
module lsu(
	input  [2:0]  lsu_op,   // NONE / LW / LBU / SW / SB, 来自 IDU
	input  [31:0] addr,     // 访存地址, 来自 EXU 的 alu_result
	input  [31:0] wdata,    // 要写入的数据, 来自 gpr 读端口 rdata2
	output [31:0] rdata     // 读出的数据, 送给 WBU
);
    // TODO(你): 实现 lw / lbu / sw / sb
    //   需要 DPI-C 调用 pmem_read / pmem_write, 并对 lbu/sb 做字节选择与 wmask
    //   目前只支持 addi, 不涉及访存, 先把输出接成确定值避免悬空
    assign rdata = 32'h0;
endmodule

