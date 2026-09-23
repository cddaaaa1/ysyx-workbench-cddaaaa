`include "define.vh"
module dpic_mem(
    input clk, 
    input rst, 
    input [31:0] ifu_raddr, 
    input ifu_reqValid, 
    output reg [31:0] ifu_rdata,
    output reg ifu_respValid,
    input [31:0] lsu_addr, 
    input [31:0] lsu_wdata, 
    input [3:0] lsu_wmask,
    input lsu_re, 
    input lsu_wen, 
    input lsu_reqValid, 
    output reg [31:0] lsu_rdata,
    output reg lsu_respValid
);
    import "DPI-C" function int  pmem_read (input int raddr);
    import "DPI-C" function void pmem_write(input int waddr, input int wdata, input byte wmask);

    reg [31:0] ifu_addr_r;
    reg [31:0] read_counter;
    reg [31:0] lsu_addr_r;
    reg [31:0] lsu_counter;
    reg        state;
    reg        lsu_state;

    // 取指: 请求那拍锁存地址, 延迟到了再把 ifu_rdata 和 ifu_respValid 一起给出
    always @(posedge clk) begin
        if (rst) begin
            state         <= `MEM_IDLE;
            read_counter  <= 32'd0;
            ifu_respValid <= 1'b0;
        end
        else begin
            ifu_respValid <= 1'b0;
            if (state == `MEM_IDLE) begin
                if (ifu_reqValid) begin
                    ifu_addr_r   <= ifu_raddr;
                    read_counter <= 32'd1;
                    state        <= `MEM_WAIT;
                end
            end
            else begin
                if (read_counter == `READ_DELAY - 1) begin
                    ifu_rdata     <= pmem_read(ifu_addr_r);
                    ifu_respValid <= 1'b1;
                    state         <= `MEM_IDLE;
                end
                else read_counter <= read_counter + 32'd1;
            end
        end
    end

    // 访存: 读延迟 READ_DELAY 拍; 写在请求那拍直接生效(不需要响应, 也保证访存顺序)
    always @(posedge clk) begin
        if (rst) begin
            lsu_state     <= `MEM_IDLE;
            lsu_counter   <= 32'd0;
            lsu_respValid <= 1'b0;
        end
        else begin
            lsu_respValid <= 1'b0;
            if (lsu_state == `MEM_IDLE) begin
                if (lsu_reqValid) begin
                    if (lsu_wen) pmem_write(lsu_addr, lsu_wdata, {4'h0, lsu_wmask});
                    else begin
                        lsu_addr_r  <= lsu_addr;
                        lsu_counter <= 32'd1;
                        lsu_state   <= `MEM_WAIT;
                    end
                end
            end
            else begin
                if (lsu_counter == `READ_DELAY-1) begin
                    lsu_rdata     <= pmem_read(lsu_addr_r);
                    lsu_respValid <= 1'b1;
                    lsu_state     <= `MEM_IDLE;
                end
                else lsu_counter <= lsu_counter + 32'd1;
            end
        end
    end
endmodule