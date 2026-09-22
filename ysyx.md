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
    ```text
    putch('A')                                    am/src/riscv/npc/trm.c
      ↓ 编译器
    lui a5,0x10000 ; sb a0,0(a5)                  ← 一条普通 store 指令
      ↓ RTL
    lsu.v: 该 sb 命中 → DPI-C pmem_write(0x10000000, 'A', 0x1)
      ↓ C++ 仿真环境
    pmem.cpp: waddr == UART_ADDR → fputc('A', stderr)
      ↓
    终端上出现 A
    MMIO 的本质: 往存储器设备区写一个字节
    ```
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
    - 验证: cd am-kernels/tests/am-tests && make ARCH=minirv-npc run mainargs=t, 程序每经过 1 秒就输出一句话
4. benchmark 输出时间或分数
    - microbench: Scored time: 3681.424 ms Total  time: 5081.065 ms
    - dhrystone: Finished in 8 ms
    - mainarg=test: Total time (ms)  : 24057
### 运行红白机游戏
1. 实现字符模式运行fceux-am 

### 通过EDA工具评估NPC的频率
1. 配置ECC, PDK , yosys 
2. 对流水灯进行综合评估 ecc run --project light 
3. 将对存储器的DPI-C访问移动到NPC外部
4. 评估NPC的综合频率：400MHZ

### 性能测试 archbench 
1. bash run-am.sh ARCH=minirv-npc mainargs=test 通过
2. bash run-am.sh ARCH=minirv-npc mainargs=train：
    ARCH      = minirv-npc
    mainargs  = train
    benchlist = 除开303.cproc  无结果
    RTC 频率   = 400 MHz  (E/minirv/csrc/pmem.cpp 的 NPC_FREQ_HZ)
    GEOMEAN   = 563 Marks   MEAN = 1542 

### E6 支持SimpleBus的IFU
1. RTL 侧
    - ifu.v: 加 idle / wait 两态状态机
      - idle: 把 pc 作为 ifu_raddr 发给存储器, 下一拍进 wait
      - wait: 存储器返回的 ifu_rdata 有效, 交给后续模块译码执行, 下一拍回 idle
      - 组合输出 ifu_valid = (state == WAIT), 顶层用它屏蔽非取指周期的状态更新
      - 关键: wait 期间 pc 保持不变, 于是同一个地址连发两拍, rdata 正好在 wait 那拍与 raddr 对上
    - pc_reg.v: 加写使能 we, 只有 ifu_valid 那拍才更新 PC
    - dpic_mem.v: 取指那一路改成寄存读, 实现 1 周期读延迟;
      数据访问那一路暂时仍是组合读 (LSU 留到下一节再改)
    - top.v: 用 ifu_valid 门控所有状态更新, 否则 idle 拍会拿上一个地址的旧数据当真去译码
      - rf_we = gpr_we && (waddr != 0) && ifu_valid; pc_we = ifu_valid
      - ebreak 的置位条件加上 ifu_valid
      - lsu 新增 valid 端口, 无效周期不产生任何访存
      - misalign 寄存一拍: 它是组合值, wait 之后立刻回 0, 不寄存仿真环境就看不到
2. DiffTest 适配: 把检查时机改成"只有一条指令执行结束才检查"
    - 一开始用顶层端口方案: top.v 把 ifu_valid 寄存一拍成 inst_valid 端口给 C++ 轮询
    - 后来改成 DPI-C import: ifu.v 里 import "DPI-C" function void sim_retire(input int pc, input int inst),
      在 state 从 WAIT 跳走的那一拍调用 (即指令退休、GPR/PC 提交的同一时刻); C++ 侧
      extern "C" void sim_retire(...) 只置一个 g_retired 标志, 主循环 if (!g_retired) continue;
      这样 top 的端口列表里就不再有为仿真需求而加的 inst_valid 了
    - 回调参数 (pc, inst) 是唯一"同源"的一对快照: 检查时 top->pc 已前进到下一条指令,
      而 top->inst 还停在退休那条, 所以出错打印要用回调存下来的 g_retire_pc / g_retire_inst
3. IPC 测量
    - 周期数 = 仿真环境 single_cycle() 的计数; 指令数 = 退休回调的次数
    - dummy: 17 条 / 34 周期; hello: 17858 条 / 35716 周期, 均为 IPC = 0.50 (2 拍/指令)
4. 验证
    - ALL=dummy / hello run → HIT GOOD TRAP + PASS; ALL=wrong run → HIT BAD TRAP + FAIL
    - prog_add: a0 = 21 (addi a0,zero,20 + add a0,a0,a1)
    - 波形: ![prog_add 的 SimpleBus 取指时序](pic/prog_add-waveform.png)
5. 性能测试： 
    - archbench-train: 
        | 日期 | commit | ARCH | mainargs | 成功/总数 | GEOMEAN | MEAN | 备注 |
        |---|---|---|---|---:|---:|---:|---|
        | 2026-09-22 14:23 | 6e6b6fe* | minirv-npc | train | 19/20 | 279 | 770 | 支持 SimpleBus 的 IFU-修改计数器溢出 |

### 

### 其他

## TODO 
- 存储器表示: REF(minirvEMU) 按字存 (`uint32_t M[]`, 字节访问靠移位+掩码),
   NPC 侧 pmem 按字节存 (`uint8_t pmem[]`, 字访问靠拼接)。对外接口都是 32 位字 + `wmask` 字节掩码, 语义等价;
- 过一遍minirv代码 + 批量运行程序
- 学习 Chisel 
- ~~archbench 现在只能跑通 11/21,其余十个有编译问题 （ai 修了~~
- archbench 303 无结果： 303.cproc 是编译器，启动就必须 fopen("input/train-Block.i") 读源文件，而 minirv-npc 平台上 FILE 这一层不可用（klib 是预编译且混淆的 fileio.o，本地没有 fileio.c 源码），于是 AM Panic: unsupport FILE → halt(1) → HIT BAD TRAP: a0=1，程序没跑到打印 [RESULT] 就结束了，所以记 0 分、显示"无结果"。