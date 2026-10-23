`include "define.vh"
module ysyx_22040000_wbu(
        input  [31:0] pc,          
        input  [1:0]  wb_sel,     
        input  [31:0] alu_result,  
        input  [31:0] mem_rdata,   
        input  [31:0] csr_rdata,   
        input         jump,        
        input  [31:0] jump_target, 
        output reg [31:0] wb_data, 
        output reg [31:0] next_pc  
);

    always @* begin
        case (wb_sel)
            `WB_ALU: wb_data = alu_result;
            `WB_MEM: wb_data = mem_rdata;
            `WB_PC4: wb_data = pc + 32'd4;
            `WB_CSR: wb_data = csr_rdata;
            default: wb_data = 32'h0;  
        endcase

        next_pc = jump ? jump_target : (pc + 32'd4);
    end
endmodule
