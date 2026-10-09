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
`define WB_CSR 2'd3

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
`define FUNCT3_CSRRS 3'b010

`define INST_EBREAK 32'h00100073

// ---- CSR 地址 ----
`define CSR_MVENDORID 12'hf11
`define CSR_MARCHID   12'hf12
`define CSR_MCYCLE    12'hb00
`define CSR_MCYCLEH   12'hb80

`define MVENDORID_VAL 32'h79737978
`define MARCHID_VAL   32'h01504dc0

// ---- IFU 取指状态机 ----
`define IFU_IDLE 1'b0
`define IFU_WAIT 1'b1
`define LSU_IDLE 1'b0
`define LSU_WAIT 1'b1
`define MEM_IDLE 1'b0
`define MEM_WAIT 1'b1
`define READ_DELAY_MIN 2

// ---- 存储器访问: Verilator 走 DPI-C, iverilog 走 VPI 注册的系统函数 ----
`ifdef __ICARUS__
`define PMEM_READ(a)      $pmem_read(a)
`define PMEM_WRITE(a,d,m) $pmem_write(a,d,m)
`else
`define PMEM_READ(a)      pmem_read(a)
`define PMEM_WRITE(a,d,m) pmem_write(a,d,m)
`endif

`ifndef PC_RESET
`ifdef YSYXSOC
`define PC_RESET 32'h30000000
`else
`define PC_RESET 32'h80000000
`endif
`endif

`endif
