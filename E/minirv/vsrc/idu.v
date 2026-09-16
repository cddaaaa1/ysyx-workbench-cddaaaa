// IDU (Instruction Decode Unit): 对当前指令译码
// 负责: 提取寄存器号, 生成立即数, 产生各部件需要的控制信号
module idu(
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

	localparam [3:0] ALU_ADDI = 4'd0; // rs1 + imm
	// 后期再加: ALU_ADD = 4'd1 (add), ALU_PASS_B = 4'd2 (lui)
	localparam [1:0] WB_ALU = 2'd0; // 运算结果 (add/addi/lui)
    localparam [1:0] WB_MEM = 2'd1; // 访存读出的数据 (lw/lbu)
    localparam [1:0] WB_PC4 = 2'd2; // jalr 的返回地址 pc+4
	localparam [2:0] LSU_NONE = 3'd0;
	localparam [6:0] OP_OPIMM = 7'b0010011;

	always_comb begin
		raddr1    = 5'b0;
		raddr2    = 5'b0;
		waddr     = 5'b0;
		gpr_we    = 1'b0;
		wb_sel    = WB_ALU;
		imm       = 32'h0;
		alu_op    = ALU_ADDI;
		is_jalr   = 1'b0;
		lsu_op    = LSU_NONE;
		is_ebreak = 1'b0;

		case (inst[6:0])
			OP_OPIMM: begin 
				raddr1 = inst[19:15];  
				waddr  = inst[11:7];
				imm    = {{20{inst[31]}}, inst[31:20]}; 
				alu_op = ALU_ADDI;
				gpr_we = 1'b1;        
			end 
			default: begin
			end
		endcase
	end 

endmodule
