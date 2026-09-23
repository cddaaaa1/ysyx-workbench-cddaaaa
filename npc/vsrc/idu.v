`include "define.vh"
module ysyx_22040000_idu(
	input  [31:0] inst,      // 当前指令
	output reg   [4:0]  raddr1,    // rs1 = inst[19:15]
	output reg   [4:0]  raddr2,    // rs2 = inst[24:20]
	output reg   [4:0]  waddr,     // rd  = inst[11:7]
	output reg          gpr_we,    // 是否需要写回 GPR
	output reg   [1:0]  wb_sel,    // 写回数据来源: ALU / 访存 / PC+4

	// 执行阶段
	output reg   [31:0] imm,       // 已符号扩展的立即数
	output reg   [3:0]  alu_op,    // ALU 控制码
	output reg          is_jalr,   // 当前指令是否 jalr

	// 访存阶段
	output reg   [2:0]  lsu_op,    // NONE / LW / LBU / SW / SB

	// 其它
	output reg          is_ebreak  // 当前指令是否 ebreak
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
				if (inst == `INST_EBREAK)
					is_ebreak = 1'b1;
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
