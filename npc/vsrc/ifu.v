`include "define.vh"
module ysyx_22040000_ifu(
        input  clk,
        input  rst,
        input  [31:0] pc,         // 来自 pc_reg: 当前 PC
        input  [31:0] ifu_rdata,  // SimpleBus: 存储器返回的指令
        input         lsu_busy,   // LSU 还在等存储器返回数据, 这拍不能发新请求
        output [31:0] inst,       // 送给 IDU, 仅当 ifu_valid 有效时才是有效指令
        output        ifu_valid,  // 本拍的 inst 是有效指令, 顶层用它屏蔽状态更新
        output [31:0] ifu_addr,   // SimpleBus: 取指地址
        input         ifu_respValid,
        output        ifu_reqValid
);
    reg state;

    always @(posedge clk) begin
        if (rst) begin
            state <= `IFU_IDLE;
        end
        else if (state == `IFU_IDLE) state <= ifu_reqValid ? `IFU_WAIT : `IFU_IDLE;
        else                         state <= ifu_respValid ? `IFU_IDLE : `IFU_WAIT;
    end

    assign ifu_reqValid = (state == `IFU_IDLE) && !lsu_busy; 
    assign ifu_valid = (state == `IFU_WAIT) && ifu_respValid;
    
    assign ifu_addr = pc;
    assign inst = ifu_rdata;
endmodule
