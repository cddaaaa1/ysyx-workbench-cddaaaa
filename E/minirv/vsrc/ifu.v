// IFU (Instruction Fetch Unit): 按 SimpleBus 协议从存储器取出一条指令
// 读出数据需要延迟一拍, 故取指占两个周期:
//   idle: 把 pc 作为取指地址发给存储器, 下一拍进 wait
//   wait: 存储器返回的指令有效, 交给后续模块译码执行, 下一拍回 idle
`include "define.vh"
module ifu(
        input  clk,
        input  rst,
        input  [31:0] pc,         // 来自 pc_reg: 当前 PC
        input  [31:0] ifu_rdata,  // SimpleBus: 存储器返回的指令
        output [31:0] inst,       // 送给 IDU, 仅当 ifu_valid 有效时才是有效指令
        output        ifu_valid,  // 本拍的 inst 是有效指令, 顶层用它屏蔽状态更新
        output [31:0] ifu_raddr   // SimpleBus: 取指地址
);
    import "DPI-C" function void sim_retire(input int pc, input int inst);

    reg state;

    always @(posedge clk) begin
        if (rst) begin
            state <= `IFU_IDLE;
        end
        else if (state == `IFU_IDLE) begin
            state <= `IFU_WAIT;
        end
        else begin // wait: 本拍的 inst 正在执行, 下一拍回 idle
            state <= `IFU_IDLE;
            sim_retire(pc, ifu_rdata); // 指令退休, 通知仿真环境可以检查 DiffTest 了
        end
    end

    assign ifu_valid = (state == `IFU_WAIT);
    // wait 期间 pc 保持不变, 相当于每拍都向存储器发同一个地址, rdata 也一直有效
    assign ifu_raddr = pc;
    assign inst = ifu_rdata;
endmodule
