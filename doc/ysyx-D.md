# ysyx 学习记录 D 阶段

## D1 从c代码到二进制程序

### 预处理

#### 观察预处理结果
- 预处理后的 `main` 函数：

  ```c
  int main() {
    printf("Hello World!\n");



    printf("RISC-V");
    return 0;
  }
  ```

- 主要变化：
  - 宏 `MSG` 被展开，反斜杠换行和注释被删除：

    ```printf(MSG /* "hi!\n" */); ```  变为： ``` printf("Hello World!\n"); ```

  - 条件编译部分因当前环境未定义 `__riscv` 而被移除。
  - `_concat(pr, intf)` 被展开为 `printf`。
  - `_str(RISC-V)` 被展开为 `"RISC-V"`。

#### 如何寻找头文件
 - `gcc -E a.c --verbose > /dev/null`
    - gcc: GNU 编译器集合，最常用来编译 C 和 C++ 代码
    - -E： 只执行预处理阶段，不进行编译、汇编和链接
    - --verbose: 输出 GCC 的详细信息
    - `> /dev/null`: ：把标准输出丢弃，不保存预处理后的 C 代码
 - 输出结果中和头文件相关的内容 
    - `/usr/lib/gcc/x86_64-linux-gnu/15/include`: GCC 自带的内部头文件目录，包含编译器相关的头文件
    - `/usr/local/include`: 本地管理员或用户安装的第三方头文件目录
    - `/usr/include/x86_64-linux-gnu`:针对当前目标平台 x86_64-linux-gnu 的系统头文件目录
    - `/user/include`: Linux 系统通用的头文件目录，stdio.h 等标准 C 库头文件通常可以在这里找到
 -  通过man gcc 阅读 GCC 手册中关于 -I 的说明
    - man gcc; 进入手册后输入 /-I ; 按回车搜索，再按 n 查看下一个匹配项
    - -I dir:  Add the directory dir to the list of directories to be searched for header  files  during  preprocessing.
 -  创建自己的stdio.h 
    -  /fake_inlcude/stdi.h 中写入 ```#define printf(...) custom_printf(__VA_ARGS__) ```
    - 然后 gcc -I./fake_include -E a.c 
    - 预处理结果``` printf()``` 变成 ``` custom_printf()```

#### 用面向RISC-V架构的gcc进行预处理
 - `riscv64-linux-gnu-gcc -E a.c` 
 - `riscv64-linux-gnu-gcc ` 预先定义了 __riscv 使`printf("Hello RISC-V!\n");`被保留

#### 对比gcc和riscv64-linux-gnu-gcc的预定义宏
- 分别保存两个编译器的预定义宏：

  ```sh
  gcc -dM -E - < /dev/null | sort > gcc.macros
  riscv64-linux-gnu-gcc -dM -E - < /dev/null | sort > riscv64-gcc.macros
  ```

- 使用 `diff` 对比两个文件：

  ```sh
  diff -u gcc.macros riscv64-gcc.macros
  ```

- 典型差异：

  ```c
  // gcc
  #define __x86_64 1
  #define __x86_64__ 1

  // riscv64-linux-gnu-gcc
  #define __riscv 1
  #define __riscv_xlen 64
  #define __riscv_i 2001000
  #define __riscv_m 2000000
  #define __riscv_a 2001000
  #define __riscv_f 2002000
  #define __riscv_d 2002000
  #define __riscv_c 2000000
  ```
  反映了两个编译器面向目标架构不同。


### 编译

##### 词法分析
- 识别并记录源文件中的每一个token

- `clang -fsyntax-only -Xclang -dump-tokens a.c`

##### 语法分析
- 识别出的token组织成树状结构

- `clang -fsyntax-only -Xclang -ast-dump a.c`
  - DeclStmt: 声明语句
  - VarDecl: 声明变量
  - DeclRefExpr: ：对已经声明的变量或函数的引用
  - CallExpr: 函数调用表达式
  - ReturnStmt: 返回语句

#### 语义分析
- `clang a.c --analyze -Xanalyzer -analyzer-output=text`

- 
  ```
    a.c:6:6: warning: Use of memory after it is freed [unix.Malloc]
    6 |   *p = 0;
      |   ~~ ^
  ```

#### 中间代码生成

#### 目标代码生成
- 使用 Clang 生成面向 RISC-V 目标平台的汇编代码：

  ```sh
  clang -S a.c --target=riscv64-linux-gnu -o a.s
  ```

- C 代码与 RISC-V 汇编的对应关系：

  ```c
  volatile int x = 10, y = 20;
  int z = x + y;
  printf("z = %d\n", z);
  return 0;
  ```

  ```asm
  li   a0, 10
  sw   a0, -24(s0)

  li   a0, 20
  sw   a0, -28(s0)
  ```

  `li` 将立即数加载到寄存器，`sw` 将 32 位整数写入栈上的局部变量位置，
  对应 `x = 10` 和 `y = 20`。

  ```asm
  lw   a0, -24(s0)
  lw   a1, -28(s0)
  addw a0, a0, a1
  sw   a0, -32(s0)
  ```

  `lw` 从内存读取 32 位整数，`addw` 执行 32 位加法，最后用 `sw` 保存结果，
  对应 `z = x + y`。

  ```asm
  lw     a1, -32(s0)
  auipc  a0, %pcrel_hi(.L.str)
  addi   a0, a0, %pcrel_lo(.Lpcrel_hi0)
  call   printf
  ```

  在 RISC-V 调用约定中，`a0` 和 `a1` 用于传递函数参数。这里 `a0` 保存格式
  字符串的地址，`a1` 保存 `z` 的值，对应 `printf("z = %d\n", z)`。

  ```asm
  .L.str:
      .asciz "z = %d\n"
  ```

  `.asciz` 定义以空字符结尾的字符串。

  ```asm
  ld   ra, 40(sp)
  ld   s0, 32(sp)
  addi sp, sp, 48
  ret
  ```

  这些指令恢复保存的寄存器、释放栈空间并返回，对应 `return 0` 以及函数结束。

- 添加 `-O1` 后的RISC64汇编后：
  - 栈空间从48字节优化到16字节 
  - 省略了帧指针s0
  - int x =10 , y =20 保留， 因为volatile
  - 直接将x+y 结果写入a1传参寄存器

### 二进制文件的生成和执行

#### 汇编
- 
  ```sh
  clang -c a.c --target=riscv64-linux-gnu
  riscv64-linux-gnu-objdump -d a.o
  ``` 

- 对比汇编和反汇编
  - a.s 中的call printf 变成实际指令 aupi ra, 0x0 jalr ra. 
  - a.o 中包含机器码
  - 

#### 链接
- 
  ```sh
  clang a.c --target=riscv64-linux-gnu -o a.out
  riscv64-linux-gnu-objdump -d a.out
  ``` 

- 链接后
  - main 地址： 
    - a.o : 0x0000
    - a.out: 0x06a2

  -  重定位
    - .L.str 的地址确定：addi a0, a0, 34 # 6f4 <_IO_stdin_used+0x4>
    - prtinf 的调用地址确定： jal 5a0 <printf@plt>

#### 执行
- `./a.out`

- 数列求和程序
  ``` c 
  int main(void) {
      int n = 1e9;   
      long long sum = 0;    

      for (int i=1; i<=n; i++){
          sum +=i;
      }
      printf("%lld\n", sum);

      return 0;
  }
  ```

- 对比编译优化后的性能差异
  - `clang sum.c -o sum.out && time ./sum.out ` ： user    0m0.053s
  - `-O0`: user    0m0.050s
  - `-O1`: user    0m0.000s 编译器直接算出了结果，优化掉了循环，程序只是在打印常量
  - `-O2`: user    0m0.000s 同上

- 程序的开始和返回
  -  GNU Debugger，即 GNU 调试器，用来观察和控制程序运行过程
   ```sh 
    clang -g sum.c -o sum.out
    gdb ./sum.out 
    (gdb) statii 
    (gdb) x/i $pc
   ```
  第一条指令： 0x7ffff7fe3340 <_start>:     mov    %rsp,%rdi  ， 保存操作系统传入的初始栈位置
  程序不由main()开始

  -  程序不是从main()返回后就结束，而是在exit_group 系统调用后结束

### 手册行为和编码规范
- 程序的行为不是只由源代码决定的，还受到语言标准、编译器、运行时环境和目标平台的共同影响。

<!-- #### 标准规范的实现

#### 程序执行的语义

#### 未指定行为
- 对于未指定行为，不同编译器会有不同的效果；

#### 实现定义行为

#### 区域特定行为

#### 未定义行为

#### 应用程序二进制接口 -->


## D2 简易调试器

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

### NEMU 代码框架

#### expr.c 表达式求值
- init_regex(): 
- make_token(): 
- expr(): 
