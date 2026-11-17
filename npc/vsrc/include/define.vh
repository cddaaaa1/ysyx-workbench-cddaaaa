`ifndef __DEFINE_VH__
`define __DEFINE_VH__

`define REG_WIDTH  4
`define CPU_WIDTH  32

// ---- ALU control codes ----
`define ALU_ADD    4'd0
`define ALU_SUB    4'd1
`define ALU_OR     4'd2
`define ALU_PASS_B 4'd3
`define ALU_EQ     4'd4
`define ALU_SLL    4'd5
`define ALU_SRL    4'd6
`define ALU_SRA    4'd7
`define ALU_AND    4'd8
`define ALU_XOR    4'd9
`define ALU_SLT    4'd10  // SLT, SLTI, BLT
`define ALU_SLTU   4'd11  // SLTU, SLTIU, BLTU
`define ALU_NE     4'd12  // BNE
`define ALU_GE     4'd13  // BGE, signed
`define ALU_GEU    4'd14  // BGEU, unsigned

// ---- ALU source select----
`define ALU_SRC1_RS1 1'd0
`define ALU_SRC1_PC  1'd1
`define ALU_SRC2_RS2 1'd0
`define ALU_SRC2_IMM  1'd1

`define RT_SRC1_RS1 1'd0
`define RT_SRC1_PC  1'd1

// ---- Writeback sources ----
`define WB_ALU 2'd0
`define WB_MEM 2'd1
`define WB_PC4 2'd2
`define WB_CSR 2'd3

// ---- Memory access control ----
`define LSU_NONE 4'd0
`define LSU_LB   4'd1
`define LSU_LH   4'd2
`define LSU_LW   4'd3
`define LSU_LBU  4'd4
`define LSU_LHU  4'd5
`define LSU_SB   4'd6
`define LSU_SH   4'd7
`define LSU_SW   4'd8

// ---- opcode ----
// `define OP_OPIMM 7'b0010011
// `define OP_JALR  7'b1100111
// `define OP_R     7'b0110011
// `define OP_LUI    7'b0110111
// `define OP_SYSTEM 7'b1110011
// `define OP_LOAD   7'b0000011
// `define OP_STORE  7'b0100011

`define OP_TYPE_R          7'b0110011  //R type
`define OP_TYPE_S          7'b0100011  //S type
`define OP_TYPE_B          7'b1100011  //B type
`define OP_TYPE_I          7'b0010011  //I type for addi/slli/srli/srai/xori/ori/andi
`define OP_TYPE_I_LOAD     7'b0000011  //I type for lb/lh/lw/lbu/lhu
`define OP_TYPE_I_JALR     7'b1100111  //I type for jalr
`define OP_TYPE_I_EBRK     7'b1110011  //I type for ecall/ebreak
`define OP_TYPE_U_LUI      7'b0110111  //U type for lui
`define OP_TYPE_U_AUIPC    7'b0010111  //U type for auipc
`define OP_TYPE_J          7'b1101111  //J type for jal
`define OP_TYPE_SYS        7'b1110011  //SYS type for ecall/ebreak/csrrw/csrrs/csrrc
`define OP_TYPE_FENCE      7'b0001111  //FENCE type for fence.i

// ---- FUNCT3
`define FUNC3_ADD_SUB           3'b000        //ADDI ADD SUB
`define FUNC3_SLL               3'b001        //SLL SLLI
`define FUNC3_SLT               3'b010        //SLT SLTI
`define FUNC3_SLTU              3'b011        //STLU STLIU
`define FUNC3_XOR               3'b100        //XOR XORI
`define FUNC3_SRL_SRA           3'b101        //SRL SRLI SRA SRAI
`define FUNC3_OR                3'b110        //OR ORI
`define FUNC3_AND               3'b111        //AND ANDI

`define FUNC3_BEQ               3'b000
`define FUNC3_BNE               3'b001
`define FUNC3_BLT               3'b100
`define FUNC3_BGE               3'b101
`define FUNC3_BLTU              3'b110
`define FUNC3_BGEU              3'b111

`define FUNCT3_LB  3'b000
`define FUNCT3_LH  3'b001
`define FUNCT3_LW  3'b010
`define FUNCT3_LBU 3'b100
`define FUNCT3_LHU 3'b101
`define FUNCT3_SW  3'b010
`define FUNCT3_SB  3'b000
`define FUNCT3_SH  3'b001

`define FUNCT3_CSRRS 3'b010

`define INST_EBREAK 32'h00100073

// ---- CSR addresses ----
`define CSR_MVENDORID 12'hf11
`define CSR_MARCHID   12'hf12
`define CSR_MCYCLE    12'hb00
`define CSR_MCYCLEH   12'hb80

`define MVENDORID_VAL `CPU_WIDTH'h79737978
`define MARCHID_VAL   `CPU_WIDTH'h01504dc0

// ---- IFU fetch state machine ----
`define IFU_IDLE 1'b0
`define IFU_WAIT 1'b1
`define LSU_IDLE 1'b0
`define LSU_WAIT 1'b1
`define MEM_IDLE 1'b0
`define MEM_WAIT 1'b1
`define READ_DELAY_MIN 2

// ---- Memory access: DPI-C for Verilator, VPI system functions for Icarus ----
`ifdef __ICARUS__
`define PMEM_READ(a)      $pmem_read(a)
`define PMEM_WRITE(a,d,m) $pmem_write(a,d,m)
`else
`define PMEM_READ(a)      pmem_read(a)
`define PMEM_WRITE(a,d,m) pmem_write(a,d,m)
`endif

`ifndef PC_RESET
`ifdef YSYXSOC
`define PC_RESET `CPU_WIDTH'h30000000
`else
`define PC_RESET `CPU_WIDTH'h80000000
`endif
`endif

`endif
