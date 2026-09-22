`ifndef __DEFINE_VH__
`define __DEFINE_VH__
// ---- ALU 控制码 ----
`define ALU_ADDI   4'd0
`define ALU_ADD    4'd1
`define ALU_PASS_B 4'd2

// ---- 写回数据来源 ----
`define WB_ALU 2'd0
`define WB_MEM 2'd1
`define WB_PC4 2'd2

// ---- 访存控制 ----
`define LSU_NONE 3'd0
`define LSU_LW  3'd1
`define LSU_LBU 3'd2
`define LSU_SW  3'd3
`define LSU_SB  3'd4

// ---- opcode ----
`define OP_OPIMM 7'b0010011
`define OP_JALR  7'b1100111
`define OP_R     7'b0110011
`define OP_LUI    7'b0110111
`define OP_SYSTEM 7'b1110011
`define OP_LOAD   7'b0000011
`define OP_STORE  7'b0100011
// ---- FUNCT3 
`define FUNCT3_LW  3'b010
`define FUNCT3_LBU 3'b100
`define FUNCT3_SW  3'b010
`define FUNCT3_SB  3'b000

`define INST_EBREAK 32'h00100073

// ---- IFU 取指状态机 ----
`define IFU_IDLE 1'b0
`define IFU_WAIT 1'b1
`endif
