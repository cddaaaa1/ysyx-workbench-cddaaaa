## D4 程序的机器级表示

### 实验工具配置

```bash 
rv32gcc() { command riscv64-linux-gnu-gcc -march=rv32im -mabi=ilp32 -ffreestanding "$@"; } 
rv64gcc() { command riscv64-linux-gnu-gcc -march=rv64im -mabi=lp64 -ffreestanding "$@"; }
rvobjdump() { command riscv64-linux-gnu-objdump "$@"; }
```

### 常数, 变量, 运算

#### 32位常数的装入

```c
int f() { return 0x123; /* 291 */ }
int g() { return -1; }
int h() { return 0x1234; /* 4660 */ }
int i() { return 0xbb8; /* 3000 */ }
```
```bash
rv32gcc -O2 -c a.c # 生成目标文件，不进行链接
rvobjdump -M no-aliases -d a.o # 对目标文件进行反汇编，查看真实指令
``` 
**汇编文本与反汇编对比**
汇编： 
```asm

f:                      # 函数入口标签
.LFB0:                  # 编译器生成的局部标签，标记函数起始位置
        .cfi_startproc  # 开始描述本函数的栈展开信息
        li      a0,291  # 把返回值 291 放进 a0
        ret             # 返回调用者
        .cfi_endproc    # 结束本函数的栈展开信息
.LFE0:                  # 编译器生成的局部标签，标记函数结束位置
        .size   f, .-f  # 记录 f 的字节大小：当前位置减去入口位置

        .align  2       # 后续位置按 2² = 4 字节对齐
        .globl  g       # 将符号 g 声明为可供其他目标文件引用
        .type   g, @function  # 将符号 g 标记为函数
```
反汇编出的实际指令
```asm 
00000000 <f>:
   0:   12300513   addi a0,zero,291
   4:   00008067   jalr zero,0(ra)
```

**反汇编结果分析**
```asm
00000000 <f>:
   0:   12300513                addi    a0,zero,291 # 直接装入正数
   4:   00008067                jalr    zero,0(ra)

00000008 <g>:
   8:   fff00513                addi    a0,zero,-1  # 直接装入负数
   c:   00008067                jalr    zero,0(ra)

00000010 <h>:
  10:   00001537                lui     a0,0x1                          # 把指令中的 20 位立即数放到寄存器高 20 位，低 12 位补零。因此 lui a0,0x1 得到的是 a0 = 0x1000 = 4096
  14:   23450513                addi    a0,a0,564 # 1234 <i+0x1218>     # 用addi补上低位
  18:   00008067                jalr    zero,0(ra)

0000001c <i>:
  1c:   00001537                lui     a0,0x1                          # 同上 a0 = 4096
  20:   bb850513                addi    a0,a0,-1096 # bb8 <i+0xb9c>     # 0xbb8 = 3000; 编译器先装入 4096，再减去 1096
  24:   00008067                jalr    zero,0(ra)
```

`addi` 的 12 位立即数按有符号数解释，范围为 $[-2048,2047]$。超出这个范围需要用`lui`。如i()中，低 12 位最高位为 1 时，`addi` 实际执行负向调整，因此 `lui` 的高位需要先加 1

#### 64位常数在RV64中的装入

```c
long long j() { return 0x1234567800000000; }
long long k() { return 0x1234567887654321; }
```

```bash
rv64gcc -O2 -S a.c  # 生成汇编文本， 不进行汇编和链接
```

```汇编文本
j:
.LFB4:
        .cfi_startproc
        li      a0,38178816  # a0 = 0x0000000002469000
        addi    a0,a0,-1329  # 先构造较短的数 a0 = 0x0000000002468acf
        slli    a0,a0,35     # 然后左移 a0 = 0x1234567800000000
        ret
        ...
k:
.LFB5:
        .cfi_startproc
        li      a5,567889920   
        li      a0,38178816  
        slli    a5,a5,2       
        addi    a0,a0,-1329  
        addi    a5,a5,801   # a5 = 0x0000000087654321
        slli    a0,a0,35    # a0 = 0x1234567800000000
        add     a0,a0,a5    # 将分别构造的两部分相加
        ret
```

#### 64位常数在RV32中的装入

```c
long long j() { return 0x1234567800000000; }
long long k() { return 0x1234567887654321; }
```

```asm 
j:
.LFB4:
        .cfi_startproc
        li      a1,305418240   
        li      a0,0          
        addi    a1,a1,1656  # RV32 用两个32位寄存器共同保存一个64位值
        ret                 # a0保存低32位, a1 保存高32位
        .cfi_endproc
```

#### 变量的大小和对齐

ABI 同时规定类型大小与对齐。下表单位均为字节；ILP32(用于RV32) 中 `int`、`long`、指针为 32 位，LP64（用于RV64） 中 `long`、指针为 64 位。

#### 变量的分配

变量最终映射到寄存器 $R$ 或内存 $M$。 $R$ 访问速度快， $M$ 访问速度慢。 
分配策略： 所有变量先分配在M,需要访问时读入$R$。

#### 程序的内存布局

| 对象 | 课件中的分配位置 |
| --- | --- |
| 全局变量 | 静态数据区 |
| 静态局部变量 | 静态数据区 |
| 非静态局部变量 | 栈；优化时也可能放入寄存器 |
| 动态分配的对象 | 堆 |

静态对象的存储需求在程序运行前确定；堆和栈支持运行时分配。

```text
  +----------+
  |          |
  +----------+
  |   stack  |
  +----------+
  |    |     |
  |    v     |
  |          |
  |    ^     |
  |    |     |
  +----------+
  |   heap   |
  +----------+
  |   data   |
  +----------+
  |   text   |
  +----------+
  |          |
0 +----------+
```

#### 变量的访问

**ILP32（RV32）**

| 类型 | 大小 | 指令 |
| --- | --- | --- |
| bool/_Bool | 1 | `lbu`/`sb` |
| char | 1 | `lbu`/`sb` |
| signed char | 1 | `lb`/`sb` |
| short | 2 | `lh`/`sh` |
| unsigned short | 2 | `lhu`/`sh` |
| int | 4 | `lw`/`sw` |
| unsigned int | 4 | `lw`/`sw` |
| long | 4 | `lw`/`sw` |
| long long | 8 | `lw+lw`/`sw+sw` |
| void * | 4 | `lw`/`sw` |
| float | 4 | `flw`/`fsw` |
| double | 8 | `fld`/`fsd` |

**LP64（RV64）**

| 类型 | 大小 | 指令 |
| --- | --- | --- |
| bool/_Bool | 1 | `lbu`/`sb` |
| char | 1 | `lbu`/`sb` |
| signed char | 1 | `lb`/`sb` |
| short | 2 | `lh`/`sh` |
| unsigned short | 2 | `lhu`/`sh` |
| int | 4 | `lw`/`sw` |
| unsigned int | 4 | `lw`/`sw` |
| long | 8 | `ld`/`sd` |
| long long | 8 | `ld`/`sd` |
| void * | 8 | `ld`/`sd` |
| float | 4 | `flw`/`fsw` |
| double | 8 | `fld`/`fsd` |

#### 运算和指令

常见 C 运算与 RISC-V 指令的对应关系：

| C运算符 | RISC-V指令 |
| --- | --- |
| `+`, `-`, `*`, `/`, `%` | `add`, `sub`, `mul`, `div`, `rem` |
| `=` | `mv`, 访存指令 |
| `&`, `\|`, `^` | `and`, `or`, `xor` |
| `~` | `xori r, r, -1` |
| `<<`, `>>` | `sll`, `srl`, `sra` |
| `!` | `sltiu r, r, 1` |
| `<`, `>` | `slt` |

#### 编译优化

```c
int sum = 0;
void f() {
  int i;
  for (i = 1; i <= 100; i ++) {
    sum += i;
  }
}
```

- `-O0`：频繁从内存取变量、把计算结果写回内存。
- `-O1`：先读入变量，在寄存器中完成循环，再统一写回。
- `-O2`：可能直接折叠循环，计算出需要累加的常数。


#### RV32 进行64位加法

```c
long long f1(long long a, long long b) { return a + b; }
```

```text
(a1, a0) <= (a1, a0) + (a3, a2)

           mv
    a1 a0 ===> a5
    a3 a2      |
               v
   carry <--- sltu
   +           ^
 ---------     |
    a1 a0 -----+
```

课件用 `sltu` 检测低位无符号加法的进位。RV64 处理 128 位加法时可以采用相同思路：

#### 有符号数和无符号数

```c
#include <stdint.h>
 int32_t add1( int32_t a,  int32_t b) { return a + b; }
uint32_t add2(uint32_t a, uint32_t b) { return a + b; }
 int32_t cmp1( int32_t a,  int32_t b) { return a < b; }
 int32_t cmp2(uint32_t a, uint32_t b) { return a < b; }
 int32_t shr1( int32_t a,  int32_t b) { return a >> b; }
uint32_t shr2(uint32_t a,  int32_t b) { return a >> b; }
 int64_t zext1( int32_t a) { return a; }
uint64_t zext2(uint32_t a) { return a; }
```

- 加法：课件中有符号与无符号版本生成相同指令，可共用加法器。
- 比较：区分 `slt` 与 `sltu`。
- 右移：区分算术右移 `sra` 与逻辑右移 `srl`。
- 扩展到更大位宽：区分符号扩展与零扩展。
- 乘法高位、除法与取余也需要考虑有无符号语义。

#### if-else

条件跳转决定接下来执行哪个代码块：

```c
int f(int x) {
  int y = 0;
  if (x > 500) y = 150;
  else if (x > 300) y = 100;
  else if (x > 100) y = 75;
  return y;
}
```

- 基本指令：`beq`、`bne`、`blt`、`bge`、`bltu`、`bgeu`。
- `bgt`、`ble`、`bgtu`、`bleu` 可通过交换比较操作数实现。
- 例如，`bgt r1,r2,offset` 等价于 `blt r2,r1,offset`。
  
#### switch-case

比较连续与稀疏 `case`，以及不同优化等级的汇编：

```c
int f(int x) {
  int y = 0;
  switch (x) {
    case 1: y = 2; break;
    case 2: case 3: y = 5; break;
    case 4: case 5: case 6: case 7: y = 8; break;
    case 8: case 9: case 10: case 11: y = 10; break;
    case 12: y = 15; break;
    default: y = -1; break;
  }
  return y;
}
```

- 跳转表：用 `x` 查分支偏移，读取后计算目标地址，再间接跳转。
- 结果表：本例在较高优化等级下，可能直接查表取得 `y`。
- 稀疏分支：若分支值为 `1`、`10`、`100` 等，构造大表不合算，可能改用比较与跳转。