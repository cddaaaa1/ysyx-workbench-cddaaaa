`include "define.vh"
module ysyx_22040000_lsu(
        input         clk,
        input         rst,
        input         valid,       
        input  [2:0]  lsu_op,      
        input  [31:0] addr,        
        input  [31:0] wdata,       
        input  [31:0] lsu_rdata,   
        output [31:0] rdata,       
        output        lsu_busy,    
        output        lsu_misalign,
        output [31:0] lsu_addr,
        output [31:0] lsu_wdata,
        output [3:0]  lsu_wmask,
        output [1:0]  lsu_size,    
        output        lsu_wen,
        output        lsu_reqValid,
        input         lsu_respValid
);  
    reg state;

    wire is_load  = valid && (lsu_op == `LSU_LW || lsu_op == `LSU_LBU);
    wire is_store = valid && (lsu_op == `LSU_SW || lsu_op == `LSU_SB);
    wire is_mem   = is_load || is_store;

    always @(posedge clk) begin
        if (rst) state <= `LSU_IDLE;
        else if (state == `LSU_IDLE) state <= is_load ? `LSU_WAIT : `LSU_IDLE;
        else                         state <= lsu_respValid ? `LSU_IDLE : `LSU_WAIT;
    end

    assign lsu_reqValid = (state == `LSU_IDLE) && is_mem; 
    assign lsu_busy     = (state == `LSU_WAIT);  

    assign lsu_addr  = is_mem ? addr : 32'h0;
    assign lsu_wen   = is_store; 
    assign lsu_size  = (is_mem && (lsu_op == `LSU_LW || lsu_op == `LSU_SW)) ? 2'b10 : 2'b00;
    assign lsu_wdata = (lsu_op == `LSU_SB) ? (wdata << (addr[1:0] * 8)) : wdata;
    assign lsu_wmask = (lsu_op == `LSU_SB) ? (4'h1 << addr[1:0])
                     : (lsu_op == `LSU_SW) ? 4'hf : 4'h0;

    assign rdata = (lsu_op == `LSU_LW)  ? lsu_rdata
                 : (lsu_op == `LSU_LBU) ? ((lsu_rdata >> (addr[1:0] * 8)) & 32'hff)
                 : 32'h0;

    assign lsu_misalign = valid && (lsu_op == `LSU_LW || lsu_op == `LSU_SW) && (addr[1:0] != 2'b0);
endmodule
