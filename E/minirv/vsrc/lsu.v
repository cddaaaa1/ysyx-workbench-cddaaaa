// LSU (Load-Store Unit): 根据控制信号访问存储器
`include "define.vh"
module lsu(
	input  [2:0]  lsu_op,   // NONE / LW / LBU / SW / SB, 来自 IDU
	input  [31:0] addr,     // 访存地址, 来自 EXU 的 alu_result
	input  [31:0] wdata,    // 要写入的数据, 来自 gpr 读端口 rdata2
	input  [31:0] dmem_rdata,
	output reg [31:0] rdata,    // 读出的数据, 送给 WBU
	output        lsu_misalign, // lw/sw 的地址未 4 字节对齐
	output reg [31:0] dmem_addr,
	output reg [31:0] dmem_wdata,
	output reg [3:0]  dmem_wmask,
	output reg        dmem_re,
	output reg        dmem_we
);

    always_comb begin 
        rdata      = 32'h0;
        dmem_addr  = 32'h0;
        dmem_wdata = 32'h0;
        dmem_wmask = 4'h0;
        dmem_re    = 1'b0;
        dmem_we    = 1'b0;
        case (lsu_op)
            `LSU_LW: begin
                dmem_re = 1'b1; 
                dmem_addr = addr;
                rdata = dmem_rdata;
            end
            `LSU_LBU: begin 
                dmem_re = 1'b1; 
                dmem_addr = addr;
                rdata = (dmem_rdata >> (addr[1:0] * 8)) & 32'hff;
            end 
            `LSU_SW: begin 
                dmem_we = 1'b1; 
                dmem_addr = addr;
                dmem_wmask = 4'hf;
                dmem_wdata = wdata;
            end 
            `LSU_SB: begin
                dmem_we = 1'b1; 
                dmem_addr = addr;
                dmem_wmask = 4'h1 << addr[1:0];
                dmem_wdata = wdata << (addr[1:0] * 8);
            end
            default: ;
        endcase
    end 

    // lw/sw 要求 4 字节对齐, lbu/sb 可用任意字节地址
    assign lsu_misalign = (lsu_op == `LSU_LW || lsu_op == `LSU_SW) && (addr[1:0] != 2'b0);

endmodule
