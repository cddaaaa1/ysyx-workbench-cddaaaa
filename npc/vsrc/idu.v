`include "define.vh"

module ysyx_26100296_idu(
    
  // ifu -> idu
  input  [31:0] inst,    

  // idu -> exu
  output reg   [4:0]  rs1_o,    
  output reg   [4:0]  rs2_o,    
  output reg   [4:0]  rd_o,     
  output reg          gpr_we_o,    
  output reg   [1:0]  wb_sel_o,    
  output reg   [31:0] imm_o,       
  output reg   [3:0]  alu_op_o,    
  output reg          is_jalr_o,   
  output reg   [2:0]  lsu_op_o,    
  output reg   [11:0] csr_addr_o,  
  output reg          is_ebreak_o

);

  wire [6:0] func7  = inst[31:25];
  wire [4:0] rs2    = inst[24:20];
  wire [4:0] rs1    = inst[19:15];
  wire [2:0] func3  = inst[14:12];
  wire [4:0] rd     = inst[11:7];
  wire [6:0] opcode = inst[6:0];

  always @(*) begin
    // Defaults
    rs1_o       = 5'b0;
    rs2_o       = 5'b0;
    rd_o        = 5'b0;
    gpr_we_o    = 1'b0;
    wb_sel_o    = `WB_ALU;
    imm_o       = 32'b0;
    alu_op_o    = `ALU_ADDI;
    is_jalr_o   = 1'b0;
    lsu_op_o    = `LSU_NONE;
    csr_addr_o  = 12'b0;
    is_ebreak_o = 1'b0;

    case (opcode)
      `OP_TYPE_R: begin
        case (func3)
          `FUNC3_ADD_SUB: begin
            case (func7)
              7'b0000000: begin
                rs1_o    = rs1;
                rs2_o    = rs2;
                rd_o     = rd;
                gpr_we_o = 1'b1;
                alu_op_o = `ALU_ADD;
              end
              default: begin end
            endcase
          end
          default: begin end
        endcase
      end

      `OP_TYPE_I: begin
        case (func3)
          `FUNC3_ADD_SUB: begin
            rs1_o    = rs1;
            rd_o     = rd;
            gpr_we_o = 1'b1;
            imm_o    = {{20{inst[31]}}, inst[31:20]};
            alu_op_o = `ALU_ADDI;
          end
          default: begin end
        endcase
      end

      `OP_TYPE_U_LUI: begin
        rd_o     = rd;
        gpr_we_o = 1'b1;
        imm_o    = {inst[31:12], 12'b0};
        alu_op_o = `ALU_PASS_B;
      end

      `OP_TYPE_I_JALR: begin
        case (func3)
          3'b000: begin
            rs1_o     = rs1;
            rd_o      = rd;
            gpr_we_o  = 1'b1;
            wb_sel_o  = `WB_PC4;
            imm_o     = {{20{inst[31]}}, inst[31:20]};
            alu_op_o  = `ALU_ADDI;
            is_jalr_o = 1'b1;
          end
          default: begin end
        endcase
      end

      `OP_TYPE_SYS: begin
        if (inst == `INST_EBREAK) begin
          is_ebreak_o = 1'b1;
        end else begin
          case (func3)
            `FUNCT3_CSRRS: begin
              rs1_o      = rs1;
              rd_o       = rd;
              gpr_we_o   = 1'b1;
              wb_sel_o   = `WB_CSR;
              csr_addr_o = inst[31:20];
            end
            default: begin end
          endcase
        end
      end

      `OP_TYPE_I_LOAD: begin
        case (func3)
          `FUNCT3_LW, `FUNCT3_LBU: begin
            rs1_o    = rs1;
            rd_o     = rd;
            gpr_we_o = 1'b1;
            wb_sel_o = `WB_MEM;
            imm_o    = {{20{inst[31]}}, inst[31:20]};
            alu_op_o = `ALU_ADDI;
            lsu_op_o = (func3 == `FUNCT3_LW) ? `LSU_LW : `LSU_LBU;
          end
          default: begin end
        endcase
      end

      `OP_TYPE_S: begin
        case (func3)
          `FUNCT3_SW, `FUNCT3_SB: begin
            rs1_o    = rs1;
            rs2_o    = rs2;
            imm_o    = {{20{inst[31]}}, inst[31:25], inst[11:7]};
            alu_op_o = `ALU_ADDI;
            lsu_op_o = (func3 == `FUNCT3_SW) ? `LSU_SW : `LSU_SB;
          end
          default: begin end
        endcase
      end

      default: begin end
    endcase
  end

endmodule
