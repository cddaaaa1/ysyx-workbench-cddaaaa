## D2 简易调试器

### NEMU 代码学习

#### 整体结构

nemu/
├── src/
│   ├── nemu-main.c         程序入口
│   ├── monitor/            启动初始化、运行控制
│   │   ├── monitor.c       参数解析、加载程序镜像、初始化各模块
│   │   └── sdb/            简易调试器
│   │       ├── sdb.c       命令解析与分发
│   │       ├── expr.c      表达式求值
│   │       └── watchpoint.c 监视点管理与检查
│   ├── engine/interpreter/ 解释器入口、结束与异常状态处理
│   ├── cpu/
│   │   ├── cpu-exec.c      指令执行循环
│   │   └── difftest/       差分测试框架与接口
│   ├── isa/
│   │   └── riscv32/        RISC-V32 专属实现
│   │       ├── init.c      初始 PC、内置程序
│   │       ├── inst.c      译码、操作数提取、指令执行
│   │       ├── reg.c       寄存器显示与按名字查询
│   │       ├── include/    ISA 相关数据结构
│   │       └── system/     中断、异常、地址翻译接口
│   ├── memory/             内存访问
│   ├── device/             外设、MMIO 等
│   └── utils/              日志、计时、反汇编、运行状态等
├── include/                公共类型、接口、宏定义
├── configs/                配置模板
├── scripts/                配置、编译、运行规则
├── tools/                  表达式生成器、配置工具等
├── Kconfig                 配置项定义
├── .config                 当前选择的配置
└── Makefile                构建入口

#### 启动流程

main() -> init_monitor() 初始化 NEMU 的运行环境 -> engine_start() 进入运行控制流程 -> sdb_mainloop （调试） 或者 cmd_c （批处理）

**`init_monitor()`**:
- `parse_args(argc, argv)`：解析启动参数，确定程序镜像、日志路径、批处理模式和 DiffTest 参数等。
- `init_rand()`：设置随机数种子
- `init_log(log_file)`：初始化日志输出
- `init_mem()`：初始化模拟物理内存
- `IFDEF(CONFIG_DEVICE, init_device())`：开启设备支持时，初始化设备及相关映射。
- `init_isa()`：进行指令集相关初始化
- `long img_size = load_img()`：若指定外部镜像，将其读入复位地址对应的模拟内存并返回镜像大小；未指定时保留内置程序
- `init_difftest(diff_so_file, img_size, difftest_port)`：初始化差分测试；未开启 DiffTest 时，该函数为空。
- `init_sdb()`：初始化简易调试器，包括编译表达式分词使用的正则表达式、初始化监视点池。
- `IFDEF(CONFIG_ITRACE, init_disasm())`：开启指令跟踪时，初始化反汇编工具，用于将机器码转换为汇编文本。
- `welcome()`：显示欢迎信息，包括指令集、跟踪开关、编译时间和帮助提示。

**`engine_start`**: 
- 启用 `CONFIG_TARGET_AM`：直接调用 `cpu_exec(-1)`
- 启用 `CONFIG_TARGET_AM`： 进入简易调试器， `sdb_mainloop()`
- `is_exit_status_bas`决定程序退出码

#### 简易调试器（sdb）

**`init_sdb()`**
- 初始化正则表达式 `init_regex()` , 初始化检查点 `init_wp_pool()`

**`sdb_mainloop()`**: 
- 读取输入 *str 
- 分离命令 *cmd , 参数 *args 
- 在cmd_table[] 中查找并调用对应处理函数; 处理函数返回负数时退出循环。

**`expr.c`**: 解析并计算表达式

**`watchpoint.c`**:  管理监视点; 

#### 指令执行循环（cpu-exec.c ）

**`cpu_exe(n)`**: 一次执行指令的入口 eg. cmd_si 调用
- 检查运行状态
- 记录开始时间
- 调用execute(n) 
- 记录结束时间
- 更新状态: NEMU_STOP 暂停， 之后可以继续执行； NEMU_ABORT: 出现异常； NEMU_END: 程序结束，输出GOODTRAP/ BADTRAP 

**`Decode`**: 
```c
typedef struct Decode {
  vaddr_t pc; // 当前指令的起始地址
  vaddr_t snpc; // 顺序执行一下条指令的地址
  vaddr_t dnpc; // 本条指令执行后去往的地址
  ISADecodeInfo isa; //机器码
  IFDEF(CONFIG_ITRACE, char logbuf[128]);
} Decode;
```

**`execute(n)`**: 
- `Decode s`: 保存处理当前一条指令所需的信息
- `exec_once (&s, cpu.pc)`: 
- `trace_and_difftest(&s,cpu.pc)`: trace，difftest 和监视点检查

**`exec_once(Decode *s, vaddr_t pc)`**

```c
  s->pc = pc;  // 保存当前指令的起始地址，处理这条指令时保持不变
  s->snpc = pc; // 设置取指起点，随后取指过程会推进 snpc
  isa_exec_once(s); //调用具体 ISA 的实现，完成取指、译码和执行，确定 dnpc
  cpu.pc = s->dnpc; // 将实际下一条指令地址写回 CPU
```


**`isa_exec_once(Decode *s)`**
- `inst_fetch(&s->snpc, 4)`: 读取机器码得到 s->isa.inst, 同时将snpc 增加4
- `decode_exec(Decode *s)`: 
  - 设置dnpc， 默认等于snpc; 跳转分支指令可以再修改
  - 匹配并执行指令
  - 保证零寄存器为0


**为什么要多个PC变量**

### PA1 - RTFSC

#### 配置系统和项目构建 
- 配置系统kconfig
  - `mconf nemu/Kconfig`
    - 作用：读取nemu/Kconfig; 打开交互式菜单配置界面；选择 ISA、编译器、优化等级、调试功能等选项； 保存后生成或更新.config
    - 报错解决： `mconf nemu/Kconfig` command not found: make menuconfig 只在项目目录中生成 nemu/tools/kconfig/build/mconf，没有将其加入系统 $PATH，因此不能直接输入 mconf，需要使用项目路径调用。
    ```sh
    cd /home/cddaaaa/Projects/ysyx/ysyx-workbench-cddaaaa/nemu
    export PATH="$PWD/tools/kconfig/build:$PATH"
    mconf Kconfig
    ```
  - `conf --syncconfig Kconfig`
    - 作用：读取已经保存的 .config；- 根据 Kconfig 检查配置是否同步；生成供 Makefile 和 C 源码使用的配置文件；
  - `make menuconfig` 会包括了以上两个步骤
- 编译和链接
  - 编译规则：nemu/scripts/build.mk

#### 准备第一个客户程序
- `src/monitor/monitor.c`:  NEMU 的监控器初始化模块，负责在 NEMU 启动时完成命令行参数解析、内存、设备、ISA、镜像和调试器等初始化工作。
- `nemu/src/isa/riscv32/init.c`: 指令集架构相关的初始化l; 加载内置测试程序 ；设置cpu初始状态；设置pc；初始化RISC-V零寄存器
- 编译并运行NEMU： 
  - `make run`
  - 进入简易调试器后键入c后, NEMU开始进入指令执行的主循环cpu_exec()
  - 客户程序已经成功地结束运行，输出： `nemu: HIT GOOD TRAP at pc = 0x8000000c`
- 为NEMU编译时添加GDB调试信息， make menuconfig , [*] Enable debug information 

#### 理解代码框架
- 理解框架代码是一个螺旋上升的过程， RTFSC 这一部分可以看作是对整体框架的初步了解，具体的代码实现将在后续实际遇到相关问题时再深入研究。 

#### 优美地推出
- 在 cmd_q() 把NEMU状态设置为NEMU_QUIT, 再return -1;这样 main() 的 is_exit_status_bad() 会把它判定为正常退出。

### PA1 - 基础设施:简易调试器 

#### 使用GDB 作为参考
- 单步执行： `next`
- 查看寄存器： `info registers`
- 查看内存： `x/4xw` ; 4: 查看四个单位； x: 十六进制显示；w： 每个单元4字节

#### 解析命令
- 再 `monitor/sdb/sdb.c`中的用法
  - 取命令： char *cmd = strtok(str, " ");
  - 取参数： char *arg = strtok(NULL, " ");

#### 单步执行
- 实现"si [N]"
- `src/monitor/sdb/sdb.c`： 命令表`cmd_table[]`中添加`si`; 添加`cmd_si()`; `sscanf` 从字符串中按格式读取数据, 这里读`%d`(一个十进制int)

#### 打印寄存器
- 实现"info r"
- `src/isa/riscv32/reg.c`：实现 `isa_reg_display(void)`; 启用`CONFIG_RVE` 时， gpr 个数为16， 关时为32
- `src/monitor/sdb/sdb.c`: 命令表`cmd_table[]`中添加`info`; 添加`cmd_info()`， 解析出`r`后调用`isa_reg_display(void)`
- 暂仅支持gpr 和 PC, 不包括CSR 

#### 扫描内存
- `sdb.c`：实现 `cmd_x()`; 使用 vaddr_read() 读内存;
- `expr.c`：输入的表达式字符串转换成一个数值； "0x80000000" -> make_token() 识别为TK_HEXNUM -> sscanf 转换为数值 0x80000000;


### 表达式求值

#### expr.c 结构
- init_regex(): 编译正则表达式；。初始化阶段执行一次，把 rules[] 中的每条正则表达式通过 regcomp() 编译成 regex_t，供后词法分析反复使用。它不处理具体表达式。

- make_token():  词法分析；输入一个字符串，从左到右逐段匹配正则规则，跳过空格，把识别出的 token 的类型和原文依次存入全局数组 tokens；

- eval(): 递归求值函数，处理 token 下标区间 [p, q]。它依次处理单个数、被括号包围的子表达式和带主运算符的子表达式：找到主运算符后，分别递归计算左右两边，最后执行 + - * / 等运算。

- expr(): 对外入口，供调试器调用。它调用 make_token() 完成词法分析，再调用 eval() 对全部 token 求值，通过 success 告诉调用者是否得到有效结果，并返回最终数值。
 

#### 实现算术表达式的词法分析和递归求值
- 添加十进制整数，加减乘除 token 以及其对应规则

- make_token()：目前目前出空格以外的token 所需行为都一致，所以switch没有任何作用; 考虑后续添加寄存器token 如 $a0 会需要解析寄存器编号这样的额外操作；暂时保留

- eval() 实现递归求值

- 顺手实现 p 命令

- 优化expr.c
  - success 的设置 
  - 检测表达式是否合规
  - 处理 `find_main_operator（）`找不到主运算符，返回-1的情况
    - 解决：在`eval()` 中添加`op==-1`的判断
  - 处理eval()中除数为零
  - 检查括号是否合规
    - `check_parentheses()`检查当前表达式三种形态
    - `PAREN_INVALID`： 括号不合法 eg: ((1+9);
    - `PAREN_NOT_ENCLOSED`: 括号合法但是表达式未被括号包括 eg:(1)+(9)
    - `PAREN_ENCLOSED`:合法且被包括 eg: (1+9)
  - 添加错误日志
  

#### 实现表达式生成器（测试用例）

- `/tools/gen-expr/gen-expr.c` 中实现gen_rand_expr()

- 修改`nemu_main.c` 跑测试， `#define TEST_EXPR 1` 跑测试， `#define TEST_EXPR 0` 正常调试器

- 编译运行: 
  - 直接 make 编译和链接
  - 运行`./build/gen-expr 10000 > input` 生成10000个测试用例到input文件中

- 如何保证表达式进行无符号运算?
  - 在生成的 C 程序中，未加后缀的十进制常量（如 4）默认是 int。因此表达式中的中间运算仍按有符号规则执行。虽然 code_format 最终把结果赋给 unsigned result，但这只会转换最终结果，无法改变中间子表达式的类型；当表达式包含除法、取模或比较时，结果可能与 NEMU 不一致。
  - 修改方案是在 gen_num() 中把格式串改为 "%uu"。数字部分由第一个 %u 输出，后面紧跟一个字符 u，例如生成 4u。这样表达式中的每个数字都是无符号常量，按照 C 的常见算术转换规则，后续所有子运算也都会按无符号规则执行。
  - 这种语义与 NEMU 一侧使用 word_t（uint32_t）保存和计算操作数的方式一致，因此参考程序得到的结果可以直接用于比较。
  - 由于 NEMU 的词法分析器不能识别 u 后缀，需要先用带 u 的表达式编译并运行参考 C 程序，再删除 buf 中的 u，最后将普通表达式写入 input 文件。当前生成表达式只包含数字、运算符和括号，因此删除所有小写 u 不会误删其他内容

- 如何随机插入空格
  - `gen_space()` 随机生成0 到2 个空格， 然后插到`gen_rand_op()`中

- 如何生成长表达式, 同时不会使buf溢出?
  - 限制`gen_rand_expr()`最大递归层数为10; 

- 如何过滤求值过程中有除0行为的表达式?
  - 不能只检查表达式里是否出现 divide by zero，因为分母可能是算出来的； 
  - 在运行时检测 C 程序是否异常终止：
    - fscanf() 可能读不到结果，返回不是 1
    - pclose() 返回非 0


<!-- ### NEMU 代码框架

#### expr.c 表达式求值
- init_regex(): 
- make_token(): 
- expr():  -->


### 监视点

#### GDB中的监视点
- break main：在 main() 开始处设置普通断点。
- run：运行程序，停在 main()。
- next：执行 int x = 0;，使局部变量 x 进入作用域。
- watch x：监视变量 x 的值，只有值发生变化时才停止。
- continue：继续运行，直到 x 的值改变。
- 再次 continue：等待下一次变化。
- info watchpoints：查看当前监视点

#### 扩展表达式求值的功能

- 寄存器 `TK_REGISTER`
  - 实现 `isa_reg_str2val()`;  
  - `eval()` 在 `p == q` 且 token 为 `TK_REGISTER` 时调用该接口；失败记录错误并返回 `false`，成功保存寄存器值并返回 `true`

- `!=` `TK_NEQ`

- `&&` `TK_AND`
  - precedence `&&` < `==` `!=` < `+` `-` < `*` `/`

- 指针解引 `TK_DEREF`
  - 当 * 不能作为乘号，也就是左侧还没有一个完整操作数时，应识别为指针解引用。
  - `**$pc`，连续的一元解引用采用右结合。
  - 在 `find_main_operator()` 中，遇到相同优先级的 TK_DEREF 时应保留最左边的解引用作为主运算符。
  - 主运算符是当前表达式的根节点，因此最左边的 * 最后执行；它的右子表达式 *$pc 先执行。

#### 监视点池管理

  **`new_wp()`**
  - `free_` 串起空闲节点，`head` 串起正在使用的节点；`new_wp()` 将节点从 `free_` 移到 `head`。
  - 取出节点时必须先保存后继，避免丢失空闲链表：

  ```c
  WP *wp = free_;
  free_ = wp->next;
  ```

  - 头插：

  ```text
  分配 A 前：free_ -> A -> B -> C；head -> X -> Y
  分配 A 后：free_ -> B -> C；head -> A -> X -> Y
  ```

    - 只需修改 `wp->next` 和 `head`，时间复杂度为 O(1)。
    - 新监视点显示在最前面，分配顺序与链表顺序相反。

  - 尾插：

  ```text
  分配 A 前：free_ -> A -> B -> C；head -> X -> Y
  分配 A 后：free_ -> B -> C；head -> X -> Y -> A
  ```

    - 需要从头遍历 `head` 找到尾节点，时间复杂度为 O(n)。

**`free_wp(WP *wp)`**
  ```
  释放前：
  head  -> A -> B -> C ; free_ -> X -> Y

  free_wp(B) 后：
  head  -> A -> C; free_ -> B -> X -> Y
  ```

  - head 删除时两种情况

> **Q：** 框架代码中定义wp_pool等变量的时候使用了关键字static, static在此处的含义是什么? 为什么要在此处使用它?
**A：** static 表示 `wp_pool`, `head`, `free` 只对`watchpoint.c`可见，其他模块只能通过想`new_wp()` 的接口函数来操作他们。没有static, 外部文件可以直接修改链表，如果操作失误可能会导致 节点丢失，链表截断等问题。 因此使用static 是将链表的维护规范到`watchpoint.c`当中，出了问题可以迅速定位到了内部的接口函数。

#### 实现监视点

**扩展监视点结构**：

```c
typedef struct watchpoint {
  int NO;// 监视点编号
  struct watchpoint *next; //链表指针
  char *e; // 表达式副本
  word_t val;// 上次的求值结果
} WP;
```
**监视点值变化检测**
- 实现`check_wp()`
  - 遍历head，在每个节点执行以下操作 
  - 重新计算表达式值
  - 与上一次的值`wp->val` 做比较
  - 如果不同，打印编号，表达是，旧值和新值；函数返回nemu_stop设置为true

**`check_wp()`接入 CPU 执行循环**
- cpu.exe.c 在`trace_difftest` 最后检查监视点
- `_check()`调用`check()` 检测到值变化设置 `nemu_state.state = NEMU_STOP;`

**实现相关SDB 命令** 
- `w EXPR` :：创建监视点；显示编号和表达式
- `info w`：打印监视点信息；显示编号、表达式和最近一次值
- `d N`：按编号删除监视点; 删除不存在的编号、负数、非数字和多余参数均有错误处理

**添加配置开关**
- Kconfig 设置宏`CONFIG_WATCHPOINT`
  ```
  config WATCHPOINT
    bool "Enable watchpoint" // 菜单选项
    depends on TARGET_NATIVE_ELF //只有当前构建目标为 native ELF 时才显示这个选项
    default n //默认关闭
    help 
      Check registered watchpoint expressions after each instruction. // 功能说明
  ```
- 通过`make menuconfig` 设置
- `IFDEF(CONFIG_WATCHPOINT, _check_wp());`

#### Q：GDB 能为段错误提供哪些有用信息？

**A：** 常见段错误包括解引用空指针、使用未初始化指针、访问已释放内存、重复释放、数组越界、写入只读内存和栈溢出。

以空指针为例：

```c
#include <stdio.h>

int main(void) {
  int *p = NULL;
  *p = 1;
  printf("%d\n", *p);
  return 0;
}
```

编译并使用 GDB 调试：

```bash
gcc -g -O0 -o a a.c
gdb -q ./a
```

GDB 输出：

```text
Program received signal SIGSEGV, Segmentation fault.
0x0000555555555161 in main () at a.c:6
6         *p = 1;
(gdb) bt
#0  0x0000555555555161 in main () at a.c:6
(gdb) print p
$1 = (int *) 0x0
(gdb) x/i $pc
=> 0x555555555161 <main+24>:    movl   $0x1,(%rax)
```

GDB 可以提供：
- run: 收到 SIGSEGV
- where:当前停在哪个函数和源码行
- print p: 打印指针类型和指向地址，(int *) 0x0 ， 说明指向空指针
- x/i $pc: pc 对应的机器指令
- bt: 显示当前的函数调用链，也就是程序是如何一步步执行到崩溃位置的

### 断点

#### Q: 如何提高断点的效率？
如果你在运行稍大一些的程序(如microbench)的时候使用断点, 你会发现设置断点之后会明显地降低NEMU执行程序的效率. 思考一下这是为什么? 

**A：** 如果用监视点模拟断点`w $pc == ADDR`，之前实现的`check_wp()`会在每条客户机指令执行后重新求值表达式， 每条都需要经过完整的表达式求值流程， microbench 通常有数百万条指令， 累计开销很大。

#### 实际调试器设置断点

工作原理：
1. 确定断点的目标位置
2. 把目标位置指令替换成陷阱指令； x86 中将目标指令替换成int3 指令
3. CPU 执行到int3 通知调试器
4. 调试器恢复原指令并暂停

### 如何阅读手册

#### 手册

- [i386 read online](https://nju-projectn.github.io/i386-manual/toc.htm)
  - Chapter 5 Memory Management - 5.1 Segment Translation - Selector

- [RISC-V Instruction Set Manual-V1-unpriv](https://github.com/riscv/riscv-isa-manual/releases/download/riscv-isa-release-382fd8b-2024-04-11/unpriv-isa-asciidoc.pdf) [RISC-V Instruction Set Manual-V2-priv](https://github.com/riscv/riscv-isa-manual/releases/download/riscv-isa-release-382fd8b-2024-04-11/priv-isa-asciidoc.pdf)


#### 必答题

**1+2...100的程序的状态机**
```
0: mov  r1, 0         |  pc0: r1 = 0;
1: mov  r2, 0         |  pc1: r2 = 0;
2: addi r2, r2, 1     |  pc2: r2 = r2 + 1;
3: add  r1, r1, r2    |  pc3: r1 = r1 + r2;
4: blt  r2, 100, 2    |  pc4: if (r2 < 100) goto pc2;   // branch if less than
5: jmp 5              |  pc5: goto pc5;
```
```
(0, x, x)
(1, 0, x)
(2, 0, 0)
第1次循环： 
(3, 0, 1)
(4, 1, 1)
(2, 1, 1)
第2次循环：
(3, 1, 2)
(4, 3, 2)
(2, 3, 2)

第99次循环
(3, 4851, 99)
(4, 4950, 99)
(2, 4950, 99)

第100次循环
(3, 4950, 100)
(4, 5050, 100)
(5, 5050, 100)
```
**riscv32有哪几种指令格式?**
4种核格式 R/I/S/, 2种立即数编码变体 B/J 

**LUI指令的行为是什么?**
lui (load upper immediate) 加载高位立即数; 指令中的 20 位立即数字段放到目标寄存器的高 20 位，低 12 位全部置零

如： lui t0, 0x12345 
执行后： t0 = 0x12345000

**mstatus寄存器的结构是怎么样的?**
在RV32 中它是一个 32 位 CSR，地址为 0x300，属于控制与状态寄存器，通过不同位域记录、控制中断使能、陷阱前的特权级等状态


**统计总行数**

统计所有 .c、.h 文件的总行数
```sh
find nemu -type f \( -name '*.c' -o -name '*.h' \) -exec cat {} + | wc -l

```
排除空行和只有空白字符的行
```sh 
find nemu -type f \( -name '*.c' -o -name '*.h' \) -exec cat {} + | grep -c '[^[:space:]]'
```

**gcc中的-Wall和-Werror有什么作用?**
-Wall: 开启常用警告，如未使用的变量、未初始化的变量使用、函数返回值类型不匹配
-Werror: 把所有的Warning当作Error