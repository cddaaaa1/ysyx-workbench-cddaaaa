## D8 支持RV32E的NPC

### 准备工作
1. 重构idu.v逻辑
2. 配置cpu-test CI 

### 为NPC搭建sdb

#### 重构NPC仿真流程

将原来集中在 `main.cpp` 的初始化、时钟驱动和运行循环拆开，借鉴 NEMU 的命令层与执行控制结构。

**执行链路**

```text
main()
├── init_monitor(argc, argv)
│   ├── parse_args() → 镜像路径、批处理选项
│   ├── pmem_load() / flash_load()
│   ├── sim_init() → 创建 contextp、top，按配置初始化波形等
│   └── sim_reset() → 复位后进入 STOP
├── sdb_mainloop()
│   ├── 交互：si / c → npc_exec(n)
│   ├── 批处理：直接 c → npc_exec(UINT64_MAX)
│   └── npc_exec → execute → exec_once → single_cycle → RTL / DPI
├── npc_statistics()
└── sim_cleanup() → 返回进程退出码
```

**对比NEMU和NPC**

| 对比 | NEMU | NPC |
|---|---|---|
| 指令语义 | C 代码完成译码、计算和状态更新 | RTL 电路完成 |
| 执行推进 | 调用指令解释函数 | C++ 驱动时钟并eval模型 |
| 一步的完成条件 | exec_once() 完成解释执行 | 观察到指令提交 |
| GPR、PC 的存放位置 | CPU_state cpu | RTL 寄存器 |
| 调试信息来源 | 软件直接取得 | 通过 DPI 等接口从 RTL 导出 |

**Q:** NEMU 和 NPC 中 exec_once()的不同语义
**A:** NEMU 中表示执行一条指令，而NPC中表示推进周期直到提交一条指令

#### NPC实现单步执行
- 验收：
`make run NPC_ARGS="" IMG=/home/cddaaaa/Projects/ysyx/ysyx-workbench-cddaaaa/am-kernels/tests/cpu-tests/build/dummy-minirv-npc.bin ARCH=minirv-npc`

#### 打印寄存器

**方法一** （未采用）
DPI-C 函数：
- RTL侧提供读取函数， 用`export "DPI-C"`导出
- C++侧包含生成的DPI头文件，使用RTL提供的读取函数
- 调用前需要设置 scope，指定读取哪一个寄存器堆实例

SDB → sim_read_gpr() → 设置 scope → RTL 读取函数 → rf

**方法二**
通过verilator 编译出的c++ 文件来访问通用寄存器

```c
static const auto &gpr_file() {   
  return top->rootp->npc_top__DOT__npc__DOT__u_gpr__DOT__rf;
}
```

### 为NPC添加DiffTest支持

**初始化+memcpy复制镜像**

nemu/
├── .config                  切换共享库目标
└── src/cpu/difftest/ref.c    实现内存复制接口

npc/
├── Makefile                 加入源码、链接 -ldl
└── csrc/
    ├── include/difftest.h   声明接口和复制方向
    ├── include/pmem.h      声明地址转换函数
    ├── pmem.cpp            实现 guest_to_host()
    ├── monitor/monitor.cpp 解析 --diff、传递镜像长度
    └── difftest/dut.cpp
        └── 加载 NEMU → 获取接口 → 初始化 → 复制镜像

```c
// nemu/src/cpu/
__EXPORT void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction) {
  // addr: NEMU 应写入的目标 guest 物理地址
  // buf: 指向 NPC 待复制数据的 host 指针
  if (direction == DIFFTEST_TO_REF) {    // 从NPC拷贝到NEMU 
    memcpy(guest_to_host(addr), buf, n);      
  } else {
    assert(0);
  }
}
``` 
**Q:** Guest地址与Host地址
**A:** Guest: 被模拟处理器使用的地址，例如 NPC/NEMU 中的物理地址 0x80000000; Host: 仿真程序访问数据的指针，例如pmem 数组的指针

**Q:** npc/dut.cpp 如何使用nemu/ref.cpp中定义的函数， difftest_memcpy，difftest_init, difftest_regcpy
**A:** 
这里使用的是动态加载共享库+函数指针调用的机制
- `dlsym(handle, "difftest_init")` : 从共享库中找到符号的地址
- `(void (*)(int))`: 转为函数指针
- `void (*ref_difftest_init)(int)`: 定义函数指针，用来保存找到的地址

**difftest_regcpy寄存器**
```
nemu/src/cpu/difftest/ref.c
└── 实现 difftest_regcpy()
    ├── TO_REF：传入状态 → NEMU 的 cpu
    └── TO_DUT：NEMU 的 cpu → 传出缓冲区

npc/csrc/
├── include/reg.h
│   └── 定义 CPUState：32 个 GPR + PC，声明 cpu_state
├── reg.cpp
│   └── 定义 cpu_state 变量
├── monitor/monitor.cpp
│   └── 将仿真初始化、复位放到 DiffTest 初始化之前
└── difftest/dut.cpp
    ├── 定义函数指针，通过 dlsym() 获取 regcpy 接口
    ├── 读取 NPC 的 GPR、PC，填入 cpu_state
    └── 调用 regcpy(&cpu_state, DIFFTEST_TO_REF)
```

```c 
__EXPORT void difftest_regcpy(void *dut, bool direction) {
  CPU_state *dut_state = (CPU_state *)dut;  // 转换指针类型，前提是双方约定的struct 布局一致
  if (direction == DIFFTEST_TO_REF) {       // NPC 状态写入 NEMU, 用于初始化
    for(int i=0; i<32; i++) {
      cpu.gpr[i] = dut_state->gpr[i];
    }
    cpu.pc = dut_state->pc;
  } else {                                  // NEMU状态写入NPC
    for(int i=0; i<32; i++) { 
      dut_state->gpr[i] = cpu.gpr[i];
    }  
    dut_state->pc = cpu.pc;
  }
}
```

**difftest_step**

```c
void difftest_step(vaddr_t pc, vaddr_t npc) {
  CPUState ref_r;
  //TODO: skip_dut_nr_instr
  //TODO: is_skip_ref

  for (int i = 0; i < 32; i++) {
    sim_read_gpr(i, cpu_state.gpr[i]);
  }
  cpu_state.pc = npc;

  ref_difftest_exec(1);
  ref_difftest_regcpy(&ref_r, DIFFTEST_TO_DUT);
  checkregs(&ref_r, pc);
}
```
**问题**：跑dummy 镜像时访存越界报错
**解决**：
1. 注释 difftest_step() 后能运行， 说明问题出现在REF部分
2. 用nemu跑dummy 没问题，说明问题出在NPC的镜像
3. 分析报错地址：0x10000003， 属于UART 控制寄存器
4. 查看 AM启动代码， _trm_init() 在 main() 前调用 uart_init()
5. 解决方案，注释掉uart_init(), 在后面的阶段在考虑适配uart


**测试**
- 在RTL中注入错误, 立即数生成大1，difftest 成功捕捉错误

- `make -C am-kernels/tests/cpu-tests ARCH=minirv-npc run DIFFTEST=0` 开启difftest,cpu-test符合预期

**问题** 切换 DIFFTEST=0/1 后，Make 未感知命令行配置变化，继续复用旧仿真程序，导致编译宏与运行参数不一致并触发 DiffTest 断言
**解决** 临时解决方案时执行`make clean`; 后续改进是增加stamp文件记录上一次的配置，并将其作为仿真程序的依赖
  

### 为NPC添加config功能 
搁置

### 为NPC添加trace


### 为NPC添加表达式求值，监视点


##### TODO:
1. sdb: 扫描内存 表达式求值，监视点
2. trace: itrace, mtrace, ftrace 
3. 统一函数声明方式，头文件还是直接放在需要使用的文件
4. makefile 流程
5. 给NPC添加config
