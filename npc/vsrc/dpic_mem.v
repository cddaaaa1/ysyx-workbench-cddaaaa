`include "define.vh"
module dpic_mem(
  input                         clk_i,
  input                         rst_i,
  input      [`CPU_WIDTH-1:0]   ifu_addr_i,
  input                         ifu_reqValid_i,
  output reg [`CPU_WIDTH-1:0]   ifu_rdata_o,
  output reg                    ifu_respValid_o,
  input      [`CPU_WIDTH-1:0]   lsu_addr_i,
  input      [`CPU_WIDTH-1:0]   lsu_wdata_i,
  input      [`CPU_WIDTH/8-1:0] lsu_wmask_i,
  input      [1:0]              lsu_size_i,
  input                         lsu_wen_i,
  input                         lsu_reqValid_i,
  output reg [`CPU_WIDTH-1:0]   lsu_rdata_o,
  output reg                    lsu_respValid_o
);
`ifndef __ICARUS__
  import "DPI-C" function int unsigned pmem_read(input int unsigned raddr);
  import "DPI-C" function void pmem_write(input int unsigned waddr, input int unsigned wdata, input byte wmask);
`endif

  reg [`CPU_WIDTH-1:0] ifu_addr_r;
  reg [`CPU_WIDTH-1:0] ifu_counter;
  reg [`CPU_WIDTH-1:0] ifu_delay;
  reg        ifu_state;
  reg [`CPU_WIDTH-1:0] lsu_addr_r;
  reg [`CPU_WIDTH-1:0] lsu_counter;
  reg [`CPU_WIDTH-1:0] lsu_delay;
  reg        lsu_state;

  reg [7:0] lfsr;
  always @(posedge clk_i) begin
    if (rst_i)                lfsr <= 8'h01;
    else if (lfsr == 8'h00) lfsr <= 8'h01;
    else                    lfsr <= {lfsr[4] ^ lfsr[3] ^ lfsr[2] ^ lfsr[0], lfsr[7:1]};
  end

  always @(posedge clk_i) begin
    if (rst_i) begin
      ifu_state     <= `MEM_IDLE;
      ifu_counter   <= `CPU_WIDTH'd0;
      ifu_respValid_o <= 1'b0;
    end
    else begin
      ifu_respValid_o <= 1'b0;
      if (ifu_state == `MEM_IDLE) begin
        if (ifu_reqValid_i) begin
          ifu_addr_r  <= ifu_addr_i;
          ifu_counter <= `CPU_WIDTH'd1;
          ifu_delay   <= `READ_DELAY_MIN + {{(`CPU_WIDTH-3){1'b0}}, lfsr[2:0]};
          ifu_state   <= `MEM_WAIT;
        end
      end
      else begin
        if (ifu_counter == ifu_delay - `CPU_WIDTH'd1) begin
          ifu_rdata_o     <= `PMEM_READ(ifu_addr_r);
          ifu_respValid_o <= 1'b1;
          ifu_state     <= `MEM_IDLE;
        end
        else ifu_counter <= ifu_counter + `CPU_WIDTH'd1;
      end
    end
  end

  always @(posedge clk_i) begin
    if (rst_i) begin
      lsu_state     <= `MEM_IDLE;
      lsu_counter   <= `CPU_WIDTH'd0;
      lsu_respValid_o <= 1'b0;
    end
    else begin
      lsu_respValid_o <= 1'b0;
      if (lsu_state == `MEM_IDLE) begin
        if (lsu_reqValid_i) begin
          if (lsu_wen_i) `PMEM_WRITE(lsu_addr_i, lsu_wdata_i, {4'h0, lsu_wmask_i});
          else begin
            lsu_addr_r  <= lsu_addr_i;
            lsu_counter <= `CPU_WIDTH'd1;
            lsu_delay   <= `READ_DELAY_MIN + {{(`CPU_WIDTH-3){1'b0}}, lfsr[2:0]};
            lsu_state   <= `MEM_WAIT;
          end
        end
      end
      else begin
        if (lsu_counter == lsu_delay - `CPU_WIDTH'd1) begin
          lsu_rdata_o     <= `PMEM_READ(lsu_addr_r);
          lsu_respValid_o <= 1'b1;
          lsu_state     <= `MEM_IDLE;
        end
        else lsu_counter <= lsu_counter + `CPU_WIDTH'd1;
      end
    end
  end
endmodule
