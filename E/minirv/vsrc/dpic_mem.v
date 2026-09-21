module dpic_mem(
    input clk, 
    input [31:0] imem_addr, output [31:0] imem_rdata,
    input [31:0] dmem_addr, input [31:0] dmem_wdata, input [3:0] dmem_wmask,
    input dmem_re, input dmem_we, output [31:0] dmem_rdata
);
    import "DPI-C" function int  pmem_read (input int raddr);
    import "DPI-C" function void pmem_write(input int waddr, input int wdata, input byte wmask);

    assign imem_rdata = pmem_read(imem_addr);
    assign dmem_rdata = dmem_re ? pmem_read(dmem_addr) : 32'h0;
    always @(posedge clk) if (dmem_we) pmem_write(dmem_addr, dmem_wdata, {4'h0, dmem_wmask});
endmodule