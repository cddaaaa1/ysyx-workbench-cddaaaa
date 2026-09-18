`ifndef __DEFINE_VH__
`define __DEFINE_VH__
// ---- ALU 控制码 (idu -> exu -> alu) ----
// `ALU_ADDI: a + imm (addi) / `ALU_ADD: a + b, b 由 EXU 取 rs2 (add) / `ALU_PASS_B: b (lui)
`define ALU_ADDI   4'd0
`define ALU_ADD    4'd1
`define ALU_PASS_B 4'd2

// ---- 写回数据来源 (idu -> wbu) ----
// `WB_ALU: 运算结果 (add/addi/lui) / `WB_MEM: 访存读出的数据 (lw/lbu) / `WB_PC4: jalr 的返回地址 pc+4
`define WB_ALU 2'd0
`define WB_MEM 2'd1
`define WB_PC4 2'd2

// ---- 访存控制 (idu -> lsu) ----
// `LSU_NONE: 不访存
`define LSU_NONE 3'd0

// ---- opcode ----
`define OP_OPIMM 7'b0010011
`define OP_R     7'b0110011
`define OP_LUI   7'b0110111

// ---- 整条指令的匹配值 ----
`define INST_EBREAK 32'h00100073
`endif
