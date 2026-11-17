`include "define.vh"

module ysyx_26100296_idu(

  // ifu -> idu
  input  [`CPU_WIDTH-1:0] inst_i,

  // idu -> exu
  output reg   [`REG_WIDTH-1:0]  rs1_o,
  output reg   [`REG_WIDTH-1:0]  rs2_o,
  output reg   [`REG_WIDTH-1:0]  rd_o,
  output reg          gpr_we_o,
  output reg   [1:0]  wb_sel_o,
  output reg   [`CPU_WIDTH-1:0] imm_o,
  output reg   [3:0]  alu_op_o,
  output reg          alu_src1_sel_o,
  output reg          alu_src2_sel_o,
  output reg          is_jump_o,
  output reg          is_jalr_o,
  output reg          is_branch_o,
  output reg          rt_src1_sel_o,
  output reg   [3:0]  lsu_op_o,
  output reg   [11:0] csr_addr_o,
  output reg          is_ebreak_o

);

  wire [6:0] func7  = inst_i[31:25];
  wire [`REG_WIDTH-1:0] rs2 = inst_i[20 +: `REG_WIDTH];
  wire [`REG_WIDTH-1:0] rs1 = inst_i[15 +: `REG_WIDTH];
  wire [2:0] func3  = inst_i[14:12];
  wire [`REG_WIDTH-1:0] rd = inst_i[7 +: `REG_WIDTH];
  wire [6:0] opcode = inst_i[6:0];

  always @(*) begin
    // Defaults
    rs1_o       = {`REG_WIDTH{1'b0}};
    rs2_o       = {`REG_WIDTH{1'b0}};
    rd_o        = {`REG_WIDTH{1'b0}};
    gpr_we_o    = 1'b0;
    wb_sel_o    = `WB_ALU;
    imm_o       = {`CPU_WIDTH{1'b0}};
    alu_op_o    = `ALU_ADD;
    alu_src1_sel_o = `ALU_SRC1_RS1;
    alu_src2_sel_o = `ALU_SRC2_RS2;
    is_jump_o   = 1'b0;
    is_jalr_o   = 1'b0;
    is_branch_o = 1'b0;
    rt_src1_sel_o = `RT_SRC1_RS1;
    lsu_op_o    = `LSU_NONE;
    csr_addr_o  = 12'b0;
    is_ebreak_o = 1'b0;

    case (opcode)
      `OP_TYPE_R: begin
        rs1_o    = rs1;
        rs2_o    = rs2;
        rd_o     = rd;
        alu_src1_sel_o = `ALU_SRC1_RS1;
        alu_src2_sel_o = `ALU_SRC2_RS2;
        case (func3)
          `FUNC3_ADD_SUB: begin
            case (func7)
              7'b0000000: begin
                gpr_we_o = 1'b1;
                alu_op_o = `ALU_ADD;
              end
              7'b0100000: begin
                gpr_we_o = 1'b1;
                alu_op_o = `ALU_SUB;
              end
              default: begin end
            endcase
          end
          `FUNC3_SLL: begin
            case (func7)
              7'b0000000: begin
                gpr_we_o = 1'b1;
                alu_op_o = `ALU_SLL;
              end
              default: begin end
            endcase
          end
          `FUNC3_SLT: begin
            case (func7)
              7'b0000000: begin
                gpr_we_o = 1'b1;
                alu_op_o = `ALU_SLT;
              end
              default: begin end
            endcase
          end
          `FUNC3_SLTU: begin
            case (func7)
              7'b0000000: begin
                gpr_we_o = 1'b1;
                alu_op_o = `ALU_SLTU;
              end
              default: begin end
            endcase
          end
          `FUNC3_XOR: begin
            case (func7)
              7'b0000000: begin
                gpr_we_o = 1'b1;
                alu_op_o = `ALU_XOR;
              end
              default: begin end
            endcase
          end
          `FUNC3_SRL_SRA: begin
            case (func7)
              7'b0000000: begin
                gpr_we_o = 1'b1;
                alu_op_o = `ALU_SRL;
              end
              7'b0100000: begin
                gpr_we_o = 1'b1;
                alu_op_o = `ALU_SRA;
              end
              default: begin end
            endcase
          end
          `FUNC3_OR: begin
            case (func7)
              7'b0000000: begin
                gpr_we_o = 1'b1;
                alu_op_o = `ALU_OR;
              end
              default: begin end
            endcase
          end
          `FUNC3_AND: begin
            case (func7)
              7'b0000000: begin
                gpr_we_o = 1'b1;
                alu_op_o = `ALU_AND;
              end
              default: begin end
            endcase
          end
          default: begin end
        endcase
      end

      `OP_TYPE_I: begin
        rs1_o    = rs1;
        rd_o     = rd;
        imm_o    = {{(`CPU_WIDTH-12){inst_i[31]}}, inst_i[31:20]};
        alu_src1_sel_o = `ALU_SRC1_RS1;
        alu_src2_sel_o = `ALU_SRC2_IMM;
        case (func3)
          `FUNC3_ADD_SUB: begin
            gpr_we_o = 1'b1;
            alu_op_o = `ALU_ADD;
          end
          `FUNC3_SLL: begin
            case (func7)
              7'b0000000: begin
                gpr_we_o = 1'b1;
                imm_o    = {{(`CPU_WIDTH-5){1'b0}}, inst_i[24:20]};
                alu_op_o = `ALU_SLL;
              end
              default: begin end
            endcase
          end
          `FUNC3_SLT: begin
            gpr_we_o = 1'b1;
            alu_op_o = `ALU_SLT;
          end
          `FUNC3_SLTU: begin
            gpr_we_o = 1'b1;
            alu_op_o = `ALU_SLTU;
          end
          `FUNC3_XOR: begin
            gpr_we_o = 1'b1;
            alu_op_o = `ALU_XOR;
          end
          `FUNC3_SRL_SRA: begin
            case (func7)
              7'b0000000: begin // SRLI
                gpr_we_o = 1'b1;
                imm_o    = {{(`CPU_WIDTH-5){1'b0}}, inst_i[24:20]};
                alu_op_o = `ALU_SRL;
              end
              7'b0100000: begin // SRAI
                gpr_we_o = 1'b1;
                imm_o    = {{(`CPU_WIDTH-5){1'b0}}, inst_i[24:20]};
                alu_op_o = `ALU_SRA;
              end
              default: begin end
            endcase
          end
          `FUNC3_OR: begin
            gpr_we_o = 1'b1;
            alu_op_o = `ALU_OR;
          end
          `FUNC3_AND: begin
            gpr_we_o = 1'b1;
            alu_op_o = `ALU_AND;
          end
          default: begin end
        endcase
      end

      `OP_TYPE_U_AUIPC, `OP_TYPE_U_LUI: begin
        rd_o     = rd;
        gpr_we_o = 1'b1;
        imm_o    = {{(`CPU_WIDTH-32){inst_i[31]}}, inst_i[31:12], 12'b0};
        alu_src2_sel_o = `ALU_SRC2_IMM;
        if (opcode == `OP_TYPE_U_AUIPC) begin
          alu_op_o = `ALU_ADD;
          alu_src1_sel_o = `ALU_SRC1_PC;
        end else begin
          alu_op_o = `ALU_PASS_B;
        end
      end

      `OP_TYPE_I_JALR: begin
        rs1_o     = rs1;
        rd_o      = rd;
        wb_sel_o  = `WB_PC4;
        imm_o     = {{(`CPU_WIDTH-12){inst_i[31]}}, inst_i[31:20]};
        alu_op_o  = `ALU_ADD;
        alu_src1_sel_o = `ALU_SRC1_RS1;
        alu_src2_sel_o = `ALU_SRC2_IMM;
        rt_src1_sel_o = `RT_SRC1_RS1;
        case (func3)
          3'b000: begin
            gpr_we_o  = 1'b1;
            is_jump_o = 1'b1;
            is_jalr_o = 1'b1;
          end
          default: begin end
        endcase
      end

      `OP_TYPE_J: begin
        rd_o     = rd;
        gpr_we_o = 1'b1;
        wb_sel_o = `WB_PC4;
        imm_o    = {{(`CPU_WIDTH-20){inst_i[31]}}, inst_i[19:12], inst_i[20], inst_i[30:21], 1'b0};
        alu_op_o = `ALU_ADD;
        alu_src1_sel_o = `ALU_SRC1_PC;
        alu_src2_sel_o = `ALU_SRC2_IMM;
        is_jump_o = 1'b1;
        rt_src1_sel_o = `RT_SRC1_PC;
      end

      `OP_TYPE_SYS: begin
        if (inst_i == `INST_EBREAK) begin
          is_ebreak_o = 1'b1;
        end else begin
          case (func3)
            `FUNCT3_CSRRS: begin
              rs1_o      = rs1;
              rd_o       = rd;
              gpr_we_o   = 1'b1;
              wb_sel_o   = `WB_CSR;
              csr_addr_o = inst_i[31:20];
            end
            default: begin end
          endcase
        end
      end

      `OP_TYPE_I_LOAD: begin
        rs1_o    = rs1;
        rd_o     = rd;
        wb_sel_o = `WB_MEM;
        imm_o    = {{(`CPU_WIDTH-12){inst_i[31]}}, inst_i[31:20]};
        alu_op_o = `ALU_ADD;
        alu_src1_sel_o = `ALU_SRC1_RS1;
        alu_src2_sel_o = `ALU_SRC2_IMM;
        case (func3)
          `FUNCT3_LB: begin
            gpr_we_o = 1'b1;
            lsu_op_o = `LSU_LB;
          end
          `FUNCT3_LH: begin
            gpr_we_o = 1'b1;
            lsu_op_o = `LSU_LH;
          end
          `FUNCT3_LW: begin
            gpr_we_o = 1'b1;
            lsu_op_o = `LSU_LW;
          end
          `FUNCT3_LBU: begin
            gpr_we_o = 1'b1;
            lsu_op_o = `LSU_LBU;
          end
          `FUNCT3_LHU: begin
            gpr_we_o = 1'b1;
            lsu_op_o = `LSU_LHU;
          end
          default: begin end
        endcase
      end

      `OP_TYPE_B: begin
        rs1_o    = rs1;
        rs2_o    = rs2;
        imm_o    = {{(`CPU_WIDTH-12){inst_i[31]}}, inst_i[7], inst_i[30:25], inst_i[11:8], 1'b0};
        alu_src1_sel_o = `ALU_SRC1_RS1;
        alu_src2_sel_o = `ALU_SRC2_RS2;
        rt_src1_sel_o = `RT_SRC1_PC;
        case (func3)
          `FUNC3_BEQ: begin
            alu_op_o = `ALU_EQ;
            is_branch_o = 1'b1;
          end
          `FUNC3_BNE: begin
            alu_op_o = `ALU_NE;
            is_branch_o = 1'b1;
          end
          `FUNC3_BLT: begin
            alu_op_o = `ALU_SLT;
            is_branch_o = 1'b1;
          end
          `FUNC3_BGE: begin
            alu_op_o = `ALU_GE;
            is_branch_o = 1'b1;
          end
          `FUNC3_BLTU: begin
            alu_op_o = `ALU_SLTU;
            is_branch_o = 1'b1;
          end
          `FUNC3_BGEU: begin
            alu_op_o = `ALU_GEU;
            is_branch_o = 1'b1;
          end
          default: begin end
        endcase
      end

      `OP_TYPE_S: begin
        rs1_o    = rs1;
        rs2_o    = rs2;
        imm_o    = {{(`CPU_WIDTH-12){inst_i[31]}}, inst_i[31:25], inst_i[11:7]};
        alu_op_o = `ALU_ADD;
        alu_src1_sel_o = `ALU_SRC1_RS1;
        alu_src2_sel_o = `ALU_SRC2_IMM;
        case (func3)
          `FUNCT3_SB: lsu_op_o = `LSU_SB;
          `FUNCT3_SH: lsu_op_o = `LSU_SH;
          `FUNCT3_SW: lsu_op_o = `LSU_SW;
          default: begin end
        endcase
      end

      default: begin end
    endcase
  end

endmodule
