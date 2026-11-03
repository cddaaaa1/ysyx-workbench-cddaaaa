## D7 工具和基础设施

### 踪迹 trace 

#### 指令执行的踪迹-itrace 

启用 `CONFIG_ITRACE` 后，`exec_once()` 根据 `s` 中保存的 PC 和机器码 `isa.inst`，结合 `disassemble()` 生成的反汇编结果，将当前指令的踪迹写入 `s->logbuf`。随后，`trace_and_difftest()` 在满足输出条件时调用 `log_write("%s\n", _this->logbuf)`，将记录写入启动参数指定的日志文件。

以当前 riscv32 配置、用户输入 `si` 为例（省略无关步骤）：
```text
main()
 ├─ init_monitor(argc, argv)
 │   ├─ parse_args()                  // 解析 -l / --log，保存日志路径
 │   ├─ init_log(log_file)
 │   │   └─ fopen(log_file, "w")      // 创建或清空日志文件，文件流保存到 log_fp
 │   └─ init_disasm()                 // CONFIG_ITRACE 开启时初始化反汇编器
 │       ├─ dlopen() / dlsym()        // 加载 Capstone 库和函数
 │       └─ cs_open_dl()              // 选择 RISC-V 架构与模式
 └─ engine_start()
     └─ sdb_mainloop()
         └─ cmd_si(NULL)              // 输入 si，默认执行 1 条指令
             └─ cpu_exec(1)
                 ├─ g_print_step = (n < 10)
                 └─ execute(1)
                     ├─ exec_once(&s, cpu.pc)
                     │   ├─ s->pc = pc; s->snpc = pc
                     │   ├─ isa_exec_once(s)
                     │   │   ├─ s->isa.inst = inst_fetch(&s->snpc, 4)
                     │   │   │                           // 保存机器码，snpc 前进 4 字节
                     │   │   └─ decode_exec(s)            // 译码、执行，确定 s->dnpc
                     │   ├─ cpu.pc = s->dnpc              // 更新实际下一条指令地址
                     │   └─ CONFIG_ITRACE 开启时
                     │       ├─ p = s->logbuf             // 每条指令从缓冲区开头写入
                     │       ├─ snprintf()                // 写入 s->pc
                     │       ├─ ilen = s->snpc - s->pc     // 计算指令长度
                     │       ├─ snprintf() / memset()     // 追加机器码和对齐空格
                     │       ├─ disassemble(...)
                     │       │   ├─ cs_disasm_dl()        // Capstone 解析机器码
                     │       │   ├─ snprintf()            // 追加助记符和操作数
                     │       │   └─ cs_free_dl()          // 释放反汇编结果
                     │       └─ iringbuf_push(s->logbuf)          // 当前新增：复制完整日志到环形缓冲区
                     ├─ g_nr_guest_inst++                // 累计执行指令数加一
                     └─ trace_and_difftest(&s, cpu.pc)
                         ├─ CONFIG_ITRACE_COND 定义且 ITRACE_COND 为真时
                         │   └─ log_write("%s\n", _this->logbuf)
                         │       └─ log_enable() 为真且 log_fp 非空时
                         │           ├─ fprintf(log_fp, ...) // 写入日志文件流
                         │           └─ fflush(log_fp)       // 刷新文件流缓冲区
                         └─ g_print_step 为真且 CONFIG_ITRACE 开启时
                             └─ puts(_this->logbuf)          // 同时在终端显示
```

#### 指令环形缓冲区-iringbuf

和itrace共用宏开关

**缓冲区**
```c
#define IRINGBUF_CAPACITY 16
#define IRINGBUF_LINE_SIZE 128
typedef struct {
  char records[IRINGBUF_CAPACITY][IRINGBUF_LINE_SIZE];  
  size_t next;    // 下次写入的下标
  size_t count;   // 记录数
} IRingBuf;
```

```c
// 写入一条指令日志
static void iringbuf_push(Decode *s) {
  snprintf(b->records[b->next], IRINGBUF_LINE_SIZE, "%s", s->logbuf);  // 将指令的日志字符串复制到 next 指向的槽位
  b->next = (b->next + 1) % IRINGBUF_CAPACITY;    // 下次写入位置，取模回绕
  if (b->count < IRINGBUF_CAPACITY) {           
    b->count ++;                                   //记录数加一，直达达到容量
  }
}

// 打印有效记录， 打印顺序为执行顺序，默认最后一条是出错指令
static void iringbuf_dump(void) { 
  if (b->count == 0) return;

  size_t start = (b->count == IRINGBUF_CAPACITY) ? b->next : 0;  // 找到最旧记录的位置
  for (size_t i = 0; i < b->count; i ++) {      
    size_t index = (start + i) % IRINGBUF_CAPACITY;      
    printf("%s\n", b->records[index]);
  }
}
```

**错误打印**
| 情况 | NEMU 如何处理 | 当前是否调用 iringbuf_dump() |
|---|---|---|
| 指令未实现或无法识别 | 设置 NEMU_ABORT | 是，在 cpu_exec() 中 |
| DiffTest 检查不一致 | 设置 NEMU_ABORT | 是，在 cpu_exec() 中 |
| 访存越界等触发 panic() / Assert() | 调用 assert_fail_msg()，随后终止进程 | 是，在 assert_fail_msg() 中 |
| NEMU 源码直接使用的小写 assert() 失败 | 直接终止进程 | 不调用 |
| 客户程序返回非零退出码，显示 HIT BAD TRAP | 设置 NEMU_END，表示客户程序报告失败 | 不会调用 |
| 正常退出、单步结束、监视点触发 | 正常结束或暂停 | 不打印 |


**测试**

- `make -C nemu/tests/iringbuf ARCH=riscv32-nemu mainargs=a run` -> ABORT
- `make -C nemu/tests/iringbuf ARCH=riscv32-nemu run` -> out of bound of pmem


#### 内存访问的踪迹 - mtrace

```c
static word_t pmem_read(paddr_t addr, int len) {
  word_t ret = host_read(guest_to_host(addr), len);

#ifdef CONFIG_MTRACE
  printf("pc=" FMT_WORD " R addr=" FMT_PADDR " len=%d data=" FMT_WORD "\n", cpu.pc, addr, len, ret);
#endif
  return ret;
}
```
- 现在会打印所有访存和取值指令，考虑后面屏蔽掉取值指令
- 只打印某一段内存区间的访问，还待添加


#### 函数调用的踪迹 - ftrace

**ELF**
readelf命令查看ELF可执行文件的信息: `riscv64-linux-gnu-readelf -a add-riscv32-nemu.elf`

Symbol Talbe(.symtab)

```
...
24: 80000010     8 FUNC    GLOBAL HIDDEN     1 main
...
```

| 内容 | 含义 |
|---|---|
| FUNC | 这个符号代表函数 |
| 80000010 | 函数起始地址 |
| 8 | 函数占 8 字节 |
| main | 函数名字 |

String Table(.strtab) 

查表：`readelf -p .strtab dummy-riscv32-nemu.elf`

上面的 main 是 readelf 帮你查出来后显示的。原始符号条目保存的不是 "main" 这几个字符，而是名字的位置

字符串表显示：[    a4] main, 意思是从字符串表开头向后移动 0xa4 字节，就找到 "main\0"。

#### ftrace 实现

  1. 给 NEMU 传入 ELF 文件
`abstract-machine/scripts/platform/nemu.mk` 中 传入 `NEMUFLAGS += -e $(IMAGE).elf`

parse_args() 解析参数和ELF 路径

  1. 读取 ELF 文件
`init_ftrace()` 解析ELF文件 构建 functions 结构体数组中

```c
  typedef struct {
    uint64_t addr;
    uint64_t size;
    char name[256];
  } Function;
```

3. 解析 ELF 文件， 得到函数列表

| 信息 | 来源 |
|---|---|
| 函数名 | 字符串表中的 st_name 偏移 |
| 起始地址 | st_value |
| 大小 | st_size |

4. 识别call和ret 
`src/isa/riscv32/inst.c`

**Call对应的实际指令**
- 直接调用: `jal ra,800001c8 <main>` 
- 函数指针调用: 
  - 源码`int x = func[0](14371, 0);`  调用的目标来自变量func中保存的函数地址
  - 跳转:`lw   a5,248(a5)` a5 得到 func[0] 中保存的函数地址, 然后`jalr ra,0(a5)` 

**Ret对应的实际指令**

- `jalr x0, 0(ra)` x0 恒为零，PC+4 写入被丢弃


**需要结合寄存器来判断**

| 指令 | rd | rs1 | 判断 |
|---|---:|---:|---|
| jal | 1（ra） | 不适用 | 调用 call |
| jal | 0（x0） | 不适用 | 普通跳转 |
| jalr | 1（ra） | 任意 | 调用 call |
| jalr | 0（x0） | 1（ra） | 当 imm == 0 时，识别为常见 ret |
| jalr | 0（x0） | 其他寄存器 | 其他跳转，可能是尾调用 |

5. 查找function名字，输出
- 由`cpu/ftrace.c`中 `ftrace_call()`和`ftrace_ret()`实现
- `ftrace_find_function_name` 查找函数名字

```c
const char *ftrace_find_function_name(uint64_t addr) {
  for (size_t i = 0; i < function_count; i ++) {
    const Function *func = &functions[i];
    if (addr == func->addr || (addr > func->addr && addr - func->addr < func->size)) { 
      //能匹配函数入口（包括大小为零的函数），也能匹配已知大小的函数内部地址，从而识别调用目标和返回指令所属的函数
        return func->name;
    }
  }
  return NULL;
}
```

**Q:** 在Linux下编写一个Hello World程序, 编译后通过上述方法找到ELF文件的字符串表, 你发现"Hello World!"字符串在字符串表中的什么位置? 为什么会这样?

**A:** 
```bash 
gcc -O0 -o hello hello.c 
./hello 
readelf -p .strtab hello  # 查看字符串表

readelf -p .rodata hello # 保存Hello world! 字符串的表
```
.strtab字符串表中没有Hellow World!字符串，.strable 通常保存函数名、变量名, 而字符串常量保存在.rodata 中


**Q:** 不匹配的函数，调用和返回：注释(2), call调用了f1, 但却从f0返回;

**A:** 
``` 
0x8000016c:    call [f2@0x800000a4]            jalr ra,0(a4)   跳进f2，保存返回地址到ra  
0x800000e8:      call [f1@0x8000005c]          jalr ra,0(a5)   跳进f1, 保存返回地址到ra,f2 返回地址进栈
                                               jalr zero,0(a5) 尾跳转到f0, 保持ra
0x80000058:      ret  [f0]                     jalr zero,0(ra) ra 为f1返回f2的地址，所以回到f2
0x800000fc:    ret  [f2]                       jalr zero,0(ra) f2 返回上一层
```

核心在f0的调用方式, f1返回的的是f0返回的值，所以编译器跳过f0返回f1, 直接通过f0返回f2 
```
return ... func[0](n - 1, l + 1);
```

**Q:** 冗余的符号表：在Linux下编写一个Hello World程序, 然后使用strip命令分别丢弃可执行文件和目标文件的符号表.symtab?
**A:**
1. 为什么可执行文件去掉符号表还能运行？
```
gcc -o hello hello.c
strip -s hello
./hello
```
生成 hello 时，链接器已经完成程序内部的符号解析和地址安排

2. 为什么目标文件去掉符号表后无法链接？
   
```
gcc -c hello.c
strip -s hello.o
gcc -o hello hello.o
```
hello.o 还没有完成链接, 还需要使用符号表.symtab

### AM作为基础设施

`ARCH=native` 时GNU/Linux提供的运行时环境，，先在 native 环境中验证软件，再到$ISA-nemu 上测试

**Q:** 阅读相关Makefile, 尝试理解abstract-machine是如何生成native的可执行文件的.
**A:** 
cpu-tests/Makefile
 └─ 生成临时 Makefile.string
     ├─ NAME = string
     ├─ SRCS = tests/string.c
     └─ include $(AM_HOME)/Makefile
         │
         ├─ 确定输出路径
         │   └─ build/string-native.elf
         │
         ├─ -include $(AM_HOME)/scripts/$(ARCH).mk
         │   └─ ARCH=native → scripts/native.mk
         │       ├─ 指定 AM 的 native 源文件
         │       ├─ 设置编译、链接参数
         │       └─ 定义 run：直接执行 $(IMAGE).elf
         │
         ├─ 编译测试程序
         │   └─ tests/string.c → string.o
         │
         ├─ 构建依赖库
         │   ├─ AM native 实现 → am-native.a
         │   └─ klib 源文件    → klib-native.a
         │
         ├─ 链接
         │   └─ string.o + 依赖库
         │       → build/string-native.elf
         │
         └─ run
             └─ Linux 执行 string-native.elf
**Ps:** .elf 的全称是 Executable and Linkable Format

### Diff test 

**Q:** 为什么需要difftest 
**A:** 一条指令执行错误，可能经过很多条指令后才表现为越界或 BAD TRAP。DiffTest 逐条比较，尽早发现状态差异

#### 代码学习

**初始化链路**
```
init_monitor()
 ├─ parse_args()
 │   └─ -d / --diff → diff_so_file
 ├─ init_isa()、load_img()
 │   └─ 准备 NEMU 的初始寄存器和程序
 └─ init_difftest()
     ├─ dlopen()                  加载 REF 动态库
     ├─ dlsym()                   获取 REF 接口函数
     ├─ ref_difftest_init()        初始化 REF
     ├─ ref_difftest_memcpy()      将程序镜像复制给 REF
     └─ ref_difftest_regcpy()      将初始寄存器复制给 REF
```
**每条指令的检查链路**
```
execute()
 ├─ exec_once()
 │   └─ NEMU 执行一条指令，更新寄存器和 cpu.pc
 └─ trace_and_difftest()
     └─ difftest_step(原指令 PC, 执行后的 PC)
         ├─ ref_difftest_exec(1)       REF 执行一条指令
         ├─ ref_difftest_regcpy()      读取 REF 状态到 ref_r
         └─ checkregs()
             └─ isa_difftest_checkregs()
                 └─ 不一致 → 设置 NEMU_ABORT
```
**DUT和REF之间的API**

``` c
void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction); // 同步内存内容

//-----------------------------------------------------------------------------------------------
void difftest_regcpy(void *dut, bool direction); //同步或读取寄存器

ref_difftest_regcpy(&cpu, DIFFTEST_TO_REF)  // 初始化时，把 NEMU（DUT） 初始寄存器状态设置给 REF
ref_difftest_regcpy(&ref_r, DIFFTEST_TO_DUT) //difftest_step()让中，将 REF 状态读取到临时变量 ref_r
//-----------------------------------------------------------------------------------------------
void difftest_exec(uint64_t n); // 让REF执行`n`条指令

//-----------------------------------------------------------------------------------------------
void difftest_init(); // 初始化REF的DiffTest功能

//-----------------------------------------------------------------------------------------------

```

**跳过检查**
对于 ebreak 这类在 REF 中执行会触发异常、行为与 NEMU 不同的指令，跳过本次 DiffTest，并将 NEMU 执行后的寄存器状态（包括 PC）同步到 REF，以 NEMU 的状态为准。

```
cpu_exec(n)
 └─ execute(n)
     ├─ exec_once()
     │   └─ isa_exec_once()
     │       └─ decode_exec()
     │           └─ 匹配 ebreak      
     │               └─ NEMUTRAP()
     │                   └─ set_nemu_state()
     │                       ├─ difftest_skip_ref()
     │                       │   └─ is_skip_ref = true
     │                       └─ 设置 NEMU_END
     │
     ├─ trace_and_difftest()
         └─ difftest_step()
             └─ 发现 is_skip_ref
                 ├─ 将 DUT 的 GPR 和 PC 同步给 REF
                 ├─ 清除跳过标志
                 └─ 返回，跳过 REF 执行和比较
```


#### Spike

**构建**

tools/difftest.mk 
 └─ 调用 tools/spike-diff/Makefile        
     └─ 生成 riscv32-spike-so
         └─ NEMU 通过 --diff 指定并加载

**ssh密钥与代理**
之前一直用HTTPS,但`tools/spike-diff/Makefile` 使用SSH下载仓库，为保持原配置配置了密钥和网络代理



**Q:** DUT 和 REF 之间需要对齐哪些内容
**A:** 
| 内容 | 何时处理 | 怎么处理 |
|---|---|---|
| 程序镜像 | 初始化时 | 将 DUT 中的程序复制到 REF 相同地址 |
| GPR 和 PC | 初始化时 | 将 cpu 状态复制给 REF |
| 执行进度 | 正常运行时 | DUT 执行一条，REF 也执行一条 |
| GPR 和 PC | 每轮执行后 | 比较两边结果，发现差异就停止 |