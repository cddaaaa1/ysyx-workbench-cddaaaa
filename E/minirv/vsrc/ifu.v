// IFU (Instruction Fetch Unit): 根据当前 PC 从存储器中取出一条指令
// PC 寄存器已独立为 pc_reg, 由顶层例化; 因此本模块是纯组合逻辑

module ifu(
        input  [31:0] pc,        // 来自 pc_reg: 当前 PC
        output [31:0] inst       // 送给 IDU
);

    // DPI-C: 调用 C++ 侧的存储器模型 (名字/签名必须与 csrc/pmem.cpp 一致)
    import "DPI-C" function int pmem_read (input int raddr);

    assign inst = pmem_read(pc);
endmodule
