`include "define.vh"
module ysyx_22040000_idu(
	input  [31:0] inst,     
	output reg   [4:0]  raddr1,    
	output reg   [4:0]  raddr2,    
	output reg   [4:0]  waddr,     
	output reg          gpr_we,    
	output reg   [1:0]  wb_sel,    
	output reg   [31:0] imm,       
	output reg   [3:0]  alu_op,    
	output reg          is_jalr,   
	output reg   [2:0]  lsu_op,    
	output reg   [11:0] csr_addr,  
	output reg          is_ebreak  
);

	always_comb begin
        raddr1    = 5'b0;
        raddr2    = 5'b0;
        waddr     = 5'b0;
        gpr_we    = 1'b0;
        wb_sel    = `WB_ALU;
        imm       = 32'h0;
        alu_op    = `ALU_ADDI;
        is_jalr   = 1'b0;
        lsu_op    = `LSU_NONE;
        csr_addr  = 12'h0;
        is_ebreak = 1'b0;

		case (inst[6:0])
			`OP_OPIMM: begin
				raddr1 = inst[19:15];
				waddr  = inst[11:7];
				imm    = {{20{inst[31]}}, inst[31:20]};
				alu_op = `ALU_ADDI;
				gpr_we = 1'b1;
			end
			`OP_R: begin
				raddr1 = inst[19:15];
				raddr2 = inst[24:20];
				waddr  = inst[11:7];
				alu_op = `ALU_ADD;
				gpr_we = 1'b1;
			end
			`OP_LUI: begin
				waddr  = inst[11:7];
				imm    = {inst[31:12], 12'b0};
				alu_op = `ALU_PASS_B;
				gpr_we = 1'b1;
			end
			`OP_JALR: begin
				raddr1 = inst[19:15];
				waddr  = inst[11:7];
				imm    = {{20{inst[31]}}, inst[31:20]};
				alu_op = `ALU_ADDI;
				gpr_we = 1'b1;
				wb_sel = `WB_PC4;
				is_jalr = 1'b1;
			end
			`OP_SYSTEM: begin
				if (inst == `INST_EBREAK) begin
					is_ebreak = 1'b1;
				end
				else if (inst[14:12] == `FUNCT3_CSRRS) begin
					raddr1   = inst[19:15];
					waddr    = inst[11:7];
					csr_addr = inst[31:20];
					gpr_we   = 1'b1;
					wb_sel   = `WB_CSR;
				end
			end
			`OP_LOAD: begin
				case (inst[14:12])
					`FUNCT3_LW: begin
						raddr1 = inst[19:15];
						waddr  = inst[11:7];
						imm    = {{20{inst[31]}}, inst[31:20]};
						alu_op = `ALU_ADDI;
						gpr_we = 1'b1;
						wb_sel = `WB_MEM;
						lsu_op = `LSU_LW;
					end
					`FUNCT3_LBU: begin
						raddr1 = inst[19:15];
						waddr  = inst[11:7];
						imm    = {{20{inst[31]}}, inst[31:20]};
						alu_op = `ALU_ADDI;
						gpr_we = 1'b1;
						wb_sel = `WB_MEM;
						lsu_op = `LSU_LBU;
					end
					default: begin
					end
				endcase
			end 
			`OP_STORE: begin
				case (inst[14:12])
					`FUNCT3_SW: begin
						raddr1 = inst[19:15];
						raddr2 = inst[24:20];
						imm    = {{20{inst[31]}}, inst[31:25], inst[11:7]};
						alu_op = `ALU_ADDI;
						lsu_op = `LSU_SW;
					end
					`FUNCT3_SB: begin
						raddr1 = inst[19:15];
						raddr2 = inst[24:20];
						imm    = {{20{inst[31]}}, inst[31:25], inst[11:7]};
						alu_op = `ALU_ADDI;
						lsu_op = `LSU_SB;
					end
					default: begin
					end
				endcase
			end
			default: begin
			end
		endcase
	end 

endmodule
