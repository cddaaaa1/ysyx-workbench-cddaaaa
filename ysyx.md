# ysyx 学习记录

# 第一周 9/8 - 9/10
1. 配置环境
2. E3 

# 第二周 9/11 - 9/17
1. RTL 仿真基础（E4）
2. 完成 NVBoard 数字电路实验（E4）
3. 实现 sCPU 与 sEMU，并进行 DiffTest（E4 完成）
4. 实现 minirv 的指令模拟器 minirvEMU（E5）
5. 用 RTL 实现 minirv NPC（E5，进行中）
6. 学习 Chisel （进行中），考虑在 D 阶段使用


# 第三周 9/18 - 9/24
### 完成用 RTL 实现 minirv NPC，与 minirvEMU 进行 DiffTest（E5 完成）
    1. RTL 实现 addi / add / lui / jalr / lw / lbu / sw / sb
    2. 加 pmem.cpp 提供 DPI-C 取指/访存
    3. 声明 DPI-C sim_ebreak(pc)：top 里用 ebreak_r 锁存, 触发条件是
       posedge clk && !rst && is_ebreak && !ebreak_r, 也就是只在首次命中 ebreak 时
       调用一次, 参数是当前 pc; 同时把 ebreak_r 输出到 ebreak 端口供仿真判断
    3. DiffTest
    4. 验证：make sim-run 跑 program/prog_*.hex;全部 difftest 通过
### 接入 AM 简易运行时环境（E6）
    1. AM 侧: minirv 工具链包装改用 riscv-none-elf-; halt() 插 ebreak 并按约定把状态码放进 a0
    2. NPC 侧: PC 复位值改 0x80000000; 存储器扩到 128MB、按基址偏移访问、改为从 .bin 装载
    3. REF 侧: minirvEMU 同步基址/容量/装载方式, 新增 mem_index() 统一地址换算
    4. 仿真侧: 支持命令行指定镜像; ebreak 时按 a0 判 HIT GOOD/BAD TRAP; 结果码回传给 make
    5. 搭建流程: npc.mk 实现 run 规则, make ARCH=minirv-npc ALL=xxx run 一键编译+仿真
    6. 验证:
        - ALL=dummy run → HIT GOOD TRAP + PASS
        - ALL=wrong run → HIT BAD TRAP + FAIL
        - riscv-tests: TEST_ISA=i 全部 PASS; ALL=addi PASS
        - 成功捕捉在NPC中注入的错误
        - 加入klib-minirv-npc.a;cpu-tests 全部通过
### 添加UART 支持字符输出
    1. 添加UART行为模型； 
        putch('A')                                    am/src/riscv/npc/trm.c
          ↓ 编译器
        lui a5,0x10000 ; sb a0,0(a5)                  ← 一条普通 store 指令
          ↓ RTL
        lsu.v: 该 sb 命中 → DPI-C pmem_write(0x10000000, 'A', 0x1)
          ↓ C++ 仿真环境
        pmem.cpp: waddr == UART_ADDR → fputc('A', stderr)
          ↓
        终端上出现 A
        (MMIO 的本质: 与"往存储器写一个字节"没有区别, 只是地址落在设备区间)
    2. 为UART添加状态寄存器 (0x10000004)
        - AM 侧 (trm.c putch): 读 0x10000004 忙等
        - 行为模型 (pmem.cpp pmem_read): raddr == 0x10000004 → (rand() & 0x7) == 0 ? 1 : 0
        - DiffTest : 同一条 load 要保证 DUT 和 REF 读到同一个数； DUT 读状态时 把这个随机值记在 g_uart_status 里; minirvEMU 执行到同一条 LBU 时取 pmem_uart_status()
        - 验证: am-kernels/kernels/hello 输出正常 + Difftest PASS
        
    3. 添加时钟支持计时功能
        - 添加时钟行为模型 (pmem.cpp)
            - 地址约定: 0x20000000 读时钟低 32 位, 0x20000004 读高 32 位
            - get_time_us(): gettimeofday() 取主机时间, 减去首次调用时的起点, 单位微秒
            - pmem_read(): 0x20000000 → g_rtc_lo = get_time_us() 的低 32 位
                          0x20000004 → g_rtc_hi = get_time_us() 的高 32 位
        - AM 侧:  __am_timer_uptime() (am/src/riscv/npc/timer.c) 
        - DiffTest: 
          - RTL 的 LW 命中 0x20000000 / 0x20000004 时, pmem_read 依次记录 g_rtc_lo / g_rtc_hi
          - minirvEMU 的 FUNCT3_LW 分支用 pmem_rtc_lo() / pmem_rtc_hi() 取回同一份值
        - 验证: cd am-kernels/tests/am-tests && make ARCH=minirv-npc run mainargs=t 实现正确的话, 程序每经过 1 秒就输出一句话
    4. benchmark 
        - microbench: Scored time: 3681.424 ms Total  time: 5081.065 ms
        - dhrystone: Finished in 8 ms
        - mainarg=test: Total time (ms)  : 24057
### 运行红白机游戏
  

### 其他


## TODO 
1. 存储器表示: REF(minirvEMU) 按字存 (`uint32_t M[]`, 字节访问靠移位+掩码),
   NPC 侧 pmem 按字节存 (`uint8_t pmem[]`, 字访问靠拼接)。对外接口都是 32 位字 + `wmask` 字节掩码, 语义等价;
2. 过一遍minirv代码 + 批量运行程序
3. 学习 Chisel 

## 进行中： 让仿真环境输出程序结束信息 （Hit bad trap 在看一遍）