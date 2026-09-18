// LSU (Load-Store Unit): 根据控制信号访问存储器
// 存储器由 C++ 实现, 通过 DPI-C 的 pmem_read / pmem_write 访问
`include "define.vh"
module lsu(
	input  [2:0]  lsu_op,   // NONE / LW / LBU / SW / SB, 来自 IDU
	input  [31:0] addr,     // 访存地址, 来自 EXU 的 alu_result
	input  [31:0] wdata,    // 要写入的数据, 来自 gpr 读端口 rdata2
	output reg [31:0] rdata,  // 读出的数据, 送给 WBU
	output lsu_misalign     // lw/sw 的地址未 4 字节对齐
);
     import "DPI-C" function int pmem_read (input int raddr);
     import "DPI-C" function void pmem_write (input int waddr, input int wdata, input byte wmask);

    always_comb begin 
        rdata = 32'h0;
        case (lsu_op)
            `LSU_LW: rdata = pmem_read(addr);
            `LSU_LBU: rdata = (pmem_read(addr) >> (addr[1:0] * 8)) & 32'hff;
            `LSU_SW: pmem_write(addr, wdata, 8'hf);
            `LSU_SB: pmem_write(addr,  wdata << (addr[1:0] * 8), 8'h1 << addr[1:0]);
            default: rdata = 32'h0; 
        endcase
    end 

    // lw/sw 要求 4 字节对齐, lbu/sb 可用任意字节地址
    assign lsu_misalign = (lsu_op == `LSU_LW || lsu_op == `LSU_SW) && (addr[1:0] != 2'b0);
endmodule
