// IFU (Instruction Fetch Unit): 根据当前 PC 从存储器中取出一条指令
// PC 寄存器已独立为 pc_reg, 由顶层例化; 因此本模块是纯组合逻辑

module ifu(
        input  [31:0] pc,        // 来自 pc_reg: 当前 PC
        input  [31:0] imem_rdata,
        output [31:0] inst,       // 送给 IDU
        output [31:0] imem_addr
);

    assign imem_addr = pc; 
    assign inst = imem_rdata;
endmodule
