# C1 ELF文件和链接

## ELF 文件

```
         链接视图 (Linking View)          执行视图 (Execution View)
         ┌──────────────────┐            ┌──────────────────┐
         │    ELF Header    │            │    ELF Header    │
         ├──────────────────┤            ├──────────────────┤
         │ Program Header   │            │ Program Header   │
         │ Table (optional) │            │ Table            │
         ├──────────────────┤            ├──────────────────┤
         │    .text         │            │                  │
         ├──────────────────┤            │   Segment 1     │
         │    .rodata       │            │   (LOAD, R+X)   │
         ├──────────────────┤            │                  │
         │    .data         │            ├──────────────────┤
         ├──────────────────┤            │                  │
         │    .bss          │            │   Segment 2     │
         ├──────────────────┤            │   (LOAD, R+W)   │
         │    .symtab       │            │                  │
         ├──────────────────┤            ├──────────────────┤
         │    .strtab       │            │ Section Header   │
         ├──────────────────┤            │ Table (optional) │
         │ Section Header   │            └──────────────────┘
         │ Table            │
         └──────────────────┘
```

### ELF 文件头

```bash
$ riscv64-linux-gnu-readelf -h build/add-riscv32e-npc.elf

ELF Header:
  Magic:   7f 45 4c 46 01 01 01 00 ...    # 魔数 "\x7fELF"
  Class:                             ELF32  # 32位
  Data:                              2's complement, little endian
  Type:                              EXEC   # 可执行文件
  Machine:                           RISC-V
  Entry point address:               0x80000000  # 入口地址 (_start)
  Start of program headers:          52
  Start of section headers:          ...
  Number of program headers:         2       # 2个加载段
  Number of section headers:         ...
```

### Section Header 节头 

```bash
$ riscv64-linux-gnu-readelf -S build/add-riscv32e-npc.elf

Section Headers:
  [Nr] Name              Type     Addr       Off    Size   ES Flg
  [ 0]                   NULL     00000000   000000 000000 00
  [ 1] .text             PROGBITS 80000000   001000 000xxx 00 AX   # 代码
  [ 2] .rodata           PROGBITS 800xxxxx   00xxxx 000xxx 00  A   # 只读数据
  [ 3] .data             PROGBITS 800xxxxx   00xxxx 000xxx 00 WA   # 可写数据
  [ 4] .bss              NOBITS   800xxxxx   00xxxx 000xxx 00 WA   # 未初始化数据
  [ 5] .symtab           SYMTAB   ...                              # 符号表
  [ 6] .strtab           STRTAB   ...                              # 字符串表
```
| 列 | 含义 | 本例 |
|---|---|---|
| Nr | 节的编号 | [1] 是 .text；对应符号表的 Ndx |
| Name | 节名称 | .text、.data 等 |
| Type | 节的类型 | PROGBITS、NOBITS 等 |
| Addr | 运行时的起始内存地址 | .text 从 0x80000000 开始 |
| Off | 相对于 ELF 文件开头的字节偏移 | .text 内容从文件的 0x1000 处开始 |
| Size | 节的大小，单位字节 | 此处 000xxx 是占位写法 |
| ES | 固定大小表项的大小 | 00 表示不适用或没有这种固定表项 |
| Flg | 节的属性标志 | AX、WA 等 |


### 符号表.symtab 

```bash
$ riscv64-linux-gnu-readelf -s build/add-riscv32e-npc.elf

Symbol table '.symtab' contains N entries:
   Num:    Value  Size Type    Bind   Vis      Ndx Name
     1: 80000000     0 NOTYPE  LOCAL  DEFAULT    1 _start
     5: 80000014    28 FUNC    GLOBAL DEFAULT    1 _trm_init
     8: 80000030    16 FUNC    GLOBAL DEFAULT    1 add
     9: 80000040    96 FUNC    GLOBAL DEFAULT    1 main
    12: 80000xxx     0 NOTYPE  GLOBAL DEFAULT    4 _heap_start
    15: 80000xxx     0 NOTYPE  GLOBAL DEFAULT  ABS _stack_pointer
```
| 列 | 含义 | 本例 |
|---|---|---|
| Num | 符号表中的编号 | 8 是符号编号，不是地址 |
| Value | 符号值；对于这里的函数，表示链接后的虚拟地址 | add 位于 0x80000030 |
| Size | 大小，单位字节；0 可以表示未记录大小 | add 大小为 16 字节 |
| Type | 类型 | FUNC 表示函数；NOTYPE 表示未指定类型 |
| Bind | 绑定属性 | LOCAL 仅本目标文件可见；GLOBAL 可供其他目标文件解析引用 |
| Vis | 可见性 | DEFAULT 表示采用默认规则，结合 Bind 判断 |
| Ndx | 所属节的编号，或特殊标记 | 1 对应节头表中的 [1]；ABS 表示绝对符号 |
| Name | 符号名称 | add、main 等 |

## 链接

**编译/汇编都无法处理跨节函数和数据的引用**
```c 
extern void f(); 
int main() {f(); return 0;}
```
```
8: 00000097  auipc ra,0x0             # ra = PC + (imm << 12)
c: 000080e7  jalr ra # 8 <main+0x8>   # PC = (ra + 0) & ~1; ra = PC + 4
```
目前是尚未重定位的机器码，需要链接器根据重定位记录确定 f 的地址，并修正调用指令中的偏移量

### 从 .c 到 .elf 的完整流程

```
  add.c          start.S        string.c (klib)
    │              │               │
    ▼ gcc -c       ▼ gcc -c        ▼ gcc -c
  add.o          start.o         string.o
    │              │               │
    │              ▼               ▼
    │         am-riscv32e-npc.a   klib-riscv32e-npc.a
    │              │               │
    └──────────────┼───────────────┘
                   │
                   ▼  ld (链接器)
            add-riscv32e-npc.elf
                   │
                   ▼  objcopy -O binary
            add-riscv32e-npc.bin
```

### 链接的过程

假设输入两个目标文件：

main.o：有 main 的代码，其中调用 add，但没有定义 add
math.o：有 add 的代码，并向外提供 add 的定义

1. 符号解析：知道 add 位于 math.o 的哪个节、节内偏移是多少
2. 组织输出节：常见为将main和math的.text合并为一个大的.text 
3. 重定位：根据地址修改机器码

#### 符号解析的过程

| 步骤 | 操作 | main.o、math.o 示例 |
|:---:|---|---|
| ① | 读取符号表和符号名 | main.o：add，GLOBAL，UND<br>math.o：add，GLOBAL，已定义 |
| ② | 登记定义状态及绑定属性 | add 有未定义引用，也有全局定义 |
| ③ | 按名称查找，按规则选择定义 | 为 main.o 引用的 add 选择 math.o 中的定义 |
| ④ | 建立关联，供后续重定位使用 | main.o 的 add 引用 → math.o 的 add 定义 |

#### 按链接脚本合并相同的节

链接脚本：`abstract-machine/scripts/linker.ld`

```
ENTRY(_start)                       // 把 _start 的地址记录为 ELF 入口
PHDRS { text PT_LOAD; data PT_LOAD; }

SECTIONS {
  /* _pmem_start and _entry_offset are defined in LDFLAGS */
  . = _pmem_start + _entry_offset;  // 从物理内存的起始地址开始
  .text : {
    *(entry)                        // 收集所有输入文件中名为 entry 的节
    *(.text*)                       // 以 .text 开头的输入节
  } : text

  _stack_top = ALIGN(0x1000);       // 把当前位置向上对齐到 4 KiB 边界，赋给 _stack_top
  . = _stack_top + 0x8000;          // 在地址布局中留出 32 KiB 的栈空间
  _stack_pointer = .;               // 3. 用 _stack_pointer 标记这块空间的高地址端
  end = .;
  _end = .;
  _heap_start = ALIGN(0x1000);
```

#### 重定位

**问题**：编译 `add.c` 时，编译器不知道 `main` 函数最终在内存哪个位置。

**解决**：
1. 编译时在指令中**留空**（填 0 或临时值）
2. 生成**重定位条目**：记录"这里需要填入某个符号的地址"
3. 链接时，链接器**确定所有符号的最终地址**，回填到指令中

**RISC-V 重定位类型**：
| 类型 | 含义 |
|------|------|
| R_RISCV_JAL | JAL 指令的 20-bit 偏移 |
| R_RISCV_BRANCH | 分支指令的 12-bit 偏移 |
| R_RISCV_HI20 | LUI/AUIPC 的高 20 位 |
| R_RISCV_LO12_I | ADDI/LW 的低 12 位 |
| R_RISCV_CALL | 函数调用 (AUIPC+JALR 组合) |

**例子**
```bash
# 查看 .o 文件中的重定位表
$ riscv64-linux-gnu-readelf -r build/riscv32e-npc/tests/add.o

Relocation section '.rela.text.check' at offset 0x4c4 contains 3 entries:
 Offset     Info    Type            Sym.Value  Sym. Name + Addend
00000014  00001813 R_RISCV_CALL_PLT  00000000   halt + 0
00000014  00000033 R_RISCV_RELAX                0
```
| 字段 | 含义 |
|---|---|
| 00000014 | 要修改的调用指令对，从 .text.check 内偏移 0x14 开始 |
| R_RISCV_CALL_PLT | 按函数调用规则处理 auipc + jalr 指令对 |
|R_RISCV_RELAX | 允许链接器优化这组指令，例如缩短为一条 jal |
| halt + 0 | 调用目标是 halt，附加数为零 |

重定位前 add.o 中的指令：
```
14: 00000097  auipc ra,0x0
18: 000080e7  jalr  ra
```
链接后的实际指令： 
```
80000024: 0c8000ef  jal 800000ec <halt>
```

### 问题合集 

**Q:.elf和.bin的关系**
1. 生成关系：链接器生成 .elf，再通过 objcopy -O binary 提取出 .bin。
2. 内容区别：
   
| 对比 | 链接后的 .elf | 生成的 .bin |
|---|---|---|
| 机器指令和数据 | 有 | 有 |
| ELF 头、节头表、程序头表 | 有 | 无 |
| 符号名，例如 main、halt | 可以保留 | 无 |
| 加载地址、入口地址等元信息 | 可以记录 | 不记录，需要运行环境约定 |

3. 实际使用：当前 NPC、NEMU 都将 .bin 加载到约定的模拟内存位置；.elf 当前在NEMU被用于在ftrace 时读取函数符号
