// WBU (WriteBack Unit): 选出写回 GPR 的数据, 并计算下一条 PC
// GPR 已独立为 gpr 模块, 由顶层例化; 因此本模块是纯组合逻辑
module wbu(
        input  [31:0] pc,          // 当前 PC
        input  [1:0]  wb_sel,      // 写回数据来源: ALU / 访存 / PC+4
        input  [31:0] alu_result,  // 来自 EXU
        input  [31:0] mem_rdata,   // 来自 LSU
        input         jump,        // 来自 EXU
        input  [31:0] jump_target, // 来自 EXU
        output reg [31:0] wb_data, // 写回 GPR 的数据 (由顶层接到 gpr.wdata)
        output reg [31:0] next_pc  // 送给 pc_reg
);
    localparam [1:0] WB_ALU = 2'd0; // 运算结果 (add/addi/lui)
    localparam [1:0] WB_MEM = 2'd1; // 访存读出的数据 (lw/lbu)
    localparam [1:0] WB_PC4 = 2'd2; // jalr 的返回地址 pc+4

    always @* begin
        case (wb_sel)
            WB_ALU: wb_data = alu_result;
            WB_MEM: wb_data = mem_rdata;
            WB_PC4: wb_data = pc + 32'd4;
            default: wb_data = 32'h0;  
        endcase

        // 跳转则取下一条 PC 为目标地址, 否则顺序执行
        next_pc = jump ? jump_target : (pc + 32'd4);
    end
endmodule
