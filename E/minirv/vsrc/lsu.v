// LSU (Load-Store Unit): 按 SimpleBus 协议访问存储器
// 写操作在发出请求的那一拍完成; 读操作数据晚一拍到, 因此 load 要多花一个周期:
//   wait_data=0: 本拍是 IFU 的 wait 拍, 把地址/写数据发给存储器
//   wait_data=1: 存储器返回的 lsu_rdata 有效, 交给 WBU 写回寄存器
`include "define.vh"
module lsu(
        input         clk,
        input         rst,
        input         valid,       // 本拍 inst 有效(IFU 的 wait 拍), 即发出访存请求
        input  [2:0]  lsu_op,      // NONE / LW / LBU / SW / SB, 来自 IDU
        input  [31:0] addr,        // 访存地址, 来自 EXU 的 alu_result
        input  [31:0] wdata,       // 要写入的数据, 来自 gpr 读端口 rdata2
        input  [31:0] lsu_rdata,   // SimpleBus: 存储器返回的读数据
        output [31:0] rdata,       // 读出的数据, 送给 WBU
        output        lsu_busy,    // 正在等存储器返回数据(指令还没结束)
        output        lsu_misalign,// lw/sw 的地址未 4 字节对齐
        output [31:0] lsu_addr,
        output [31:0] lsu_wdata,
        output [3:0]  lsu_wmask,
        output        lsu_re,
        output        lsu_wen
);
    wire is_load  = valid && (lsu_op == `LSU_LW || lsu_op == `LSU_LBU);
    wire is_store = valid && (lsu_op == `LSU_SW || lsu_op == `LSU_SB);

    // 发出读请求后, 下一拍等存储器把数据送回来
    reg wait_data;
    always @(posedge clk) begin
        if (rst) wait_data <= 1'b0;
        else     wait_data <= is_load;
    end
    assign lsu_busy = wait_data;

    // 地址/控制/写数据只在真正发起访存请求时才有效, 其余周期给 0。
    // 注意这里必须用 is_load/is_store 而不是 valid:
    //   存储器模型里的 pmem_read 是 DPI-C 调用, Verilator 会把它从三元表达式
    //   `lsu_re ? pmem_read(lsu_addr) : 0` 里提出来无条件执行。若只在取指的 wait 拍
    //   (valid=1)才屏蔽地址, 那么执行非访存指令时也会把 ALU 的结果(对它们来说是
    //   无意义的中间值, 比如 -32768 / -4)当地址送出去, 造成一堆无效的越界访问。
    assign lsu_addr  = (is_load || is_store) ? addr : 32'h0;
    assign lsu_re    = is_load;
    assign lsu_wen   = is_store;
    assign lsu_wdata = (lsu_op == `LSU_SB) ? (wdata << (addr[1:0] * 8)) : wdata;
    assign lsu_wmask = (lsu_op == `LSU_SB) ? (4'h1 << addr[1:0])
                     : (lsu_op == `LSU_SW) ? 4'hf : 4'h0;

    // 送给 WBU 的数据: 只有 load 才真的用存储器数据。
    // 存储器把读数据寄存了一拍(见 dpic_mem.v), 所以 lsu_busy 拉高那拍 lsu_rdata
    // 已经有效; 此时 IFU 还停在同一条指令上(pc 没推进), lsu_op / addr 依然有效,
    // 可以直接拿来选字/选字节。
    assign rdata = (lsu_op == `LSU_LW)  ? lsu_rdata
                 : (lsu_op == `LSU_LBU) ? ((lsu_rdata >> (addr[1:0] * 8)) & 32'hff)
                 : 32'h0;

    // lw/sw 要求 4 字节对齐, lbu/sb 可用任意字节地址
    assign lsu_misalign = valid && (lsu_op == `LSU_LW || lsu_op == `LSU_SW) && (addr[1:0] != 2'b0);

endmodule
