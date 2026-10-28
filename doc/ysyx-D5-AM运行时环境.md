## D5 AM 运行时环境 
(PA2.3 程序, 运行时环境与AM) 

### AM 设计哲学

```
                    ┌────────────────────────────────────────┐
                    │        应用程序 (Application)           │
                    │   typing-game / snake / CoreMark ...   │
                    └──────────────────┬─────────────────────┘
                                       │ 调用统一 API
                    ┌──────────────────▼─────────────────────┐
                    │          Abstract-Machine (am.h)       │
                    │                                        │
                    │  TRM    IOE    CTE    VME    MPE       │
                    └──────────────────┬─────────────────────┘
                                       │ 分平台实现
              ┌────────────┬───────────┼───────────┬────────────┐
              ▼            ▼           ▼           ▼            ▼
         NEMU 平台    NPC 平台    ysyxSoC     QEMU 平台    Native
        (ioe.c...)   (ioe.c...)   (FPGA)     (ioe.c...)   (Linux)
```
AM 将程序运行所需的功能封装成统一的库函数接口，由各架构分别实现，让应用程序无需关心底层硬件差异，实现程序与架构的解耦。

#### 五层抽象
| 层级 | 名称 | 功能 | 
|------|------|------|
| **TRM** | 图灵机 | 最基本的计算 (内存 + 执行 + 停机) | 
| **IOE** | I/O 扩展 | 设备抽象 (定时器/键盘/显示/音频) | 
| **CTE** | 上下文扩展 | 中断/异常处理, 上下文切换 |
| **VME** | 虚拟内存 | 页表管理, 地址空间隔离 | 
| **MPE** | 多处理 | 多核支持, 原子操作 | 


#### 代码框架

| 部分 | 作用 |
|---|---|
| am/ | 提供 AM 接口及其架构相关实现 | 
| klib/ | 提供字符串、格式化输出等常用 C 库功能 | 
| scripts/ 与根 Makefile | 选择源码、编译、链接、生成镜像并启动 NEMU |  


### TRM: Turing Machine 

```c
extern   Area        heap;      //记录可用堆内存的起止地址
void     putch       (char ch); //输出一个字符
void     halt        (int code) __attribute__((__noreturn__)); //结束程序运行，code 为结束码
```

### AM 程序的构建与运行

#### 构建流程

程序编译出来后，必须适合它将要运行的环境: *默认编译* 生成的是面向当前GNU/Linux 环境的程序。所以需要*交叉编译* 在Linux主机上生成供`riscv32-nemu` 运行的程序。

```
AM 源码 ─────> 多个 .o ──> am-riscv32-nemu.a ────┐
客户程序 dummy.c ────────> dummy.o ──────────────┤
klib 源码 ───> 多个 .o ──> klib-riscv32-nemu.a ──┤
                                                │
                                                ▼
                                            ld 链接
                                                │
                                                ▼
                                  dummy-riscv32-nemu.elf
                                                │
                                      objcopy 生成镜像
                                                │
                                                ▼
                                  dummy-riscv32-nemu.bin
                                                │
                                                ▼
                                       NEMU 加载并执行
```

#### Makefile 公共构建规则

AM，客户程序，klib 分别提供自己的 NAME、SRCS，然后复用 abstract-machine/Makefile 中的公共构建规则

**输出路径与文件名**
```makefile 
### Create the destination directory (`build/$ARCH`)
WORK_DIR  = $(shell pwd)                # 这次Make 的工作目录
DST_DIR   = $(WORK_DIR)/build/$(ARCH)   # 存放目标文件.o 的目录
$(shell mkdir -p $(DST_DIR))           

### Compilation targets (a binary image or archive)
IMAGE_REL = build/$(NAME)-$(ARCH)       
IMAGE     = $(abspath $(IMAGE_REL))     # 最终程序文件的路径前缀
ARCHIVE   = $(WORK_DIR)/build/$(NAME)-$(ARCH).a # 静态库 .a 的完整路径
```
以AM的构建为例:
```makefile 
WORK_DIR = abstract-machine/am
NAME     = am
ARCH     = riscv32-nemu

DST_DIR  = abstract-machine/am/build/riscv32-nemu
ARCHIVE  = abstract-machine/am/build/am-riscv32-nemu.a
```

**源文件编译为目标文件**

```makefile 
$(DST_DIR)/%.o: %.c
    @$(CC) -std=gnu17 $(CFLAGS) -c -o $@ $(realpath $<)
```
- -c: 生成目标文件，暂不链接
- $<：当前规则的第一个依赖，即输入源文件 
- $@：当前目标，即输出的 .o 文件 
- std=gnu17：指定使用 C17 标准，并允许 GNU 扩展
- $(CC): 交叉编译器，RISC-V配置下为`riscv-none-elf-gcc` , 定义于`abstract-machine/scripts/isa/riscv.mk`

**运行库的构建与归档**

```makefile 
ifeq ($(MAKECMDGOALS),archive)  # 归档分支：把本次构建的目标文件打包
### Rule (archive): objects (`*.o`) -> `ARCHIVE.a` (ar)
$(ARCHIVE): $(OBJS)            
	@echo + AR "->" $(shell realpath $@ --relative-to .) 
	@$(AR) rcs $@ $^
else                            
# $(1): library name                              # 定义一个生成库规则的模板                             
define LIB_TEMPLATE =         
$$(AM_HOME)/$(1)/build/$(1)-$$(ARCH).a: force
	@$$(MAKE) -s -C $$(AM_HOME)/$(1) archive  
LINKAGE += $$(AM_HOME)/$(1)/build/$(1)-$$(ARCH).a # 加入链接列表
endef
### Rule (recursive make): build a dependent library (am, klib, ...)
$(foreach lib, $(LIBS), $(eval $(call LIB_TEMPLATE,$(lib))))  
endif
```

- $(OBJS):它依赖的所有 .o 文件
- $(AR): 归档工具，`riscv-none-elf-ar`
- r: 添加目标文件
- c: 允许创建归档
- s: 建立符号索引


**示例：构建 dummy 所需的 AM 库**

1. 客户程序的make run 进入else, 定义LIB_TEMPLATE
2. foreach 遍历到 am，生成 AM 库的规则, 相当于写下： 
```makefile 
$(AM_HOME)/am/build/am-riscv32-nemu.a: force
	@$(MAKE) -s -C $(AM_HOME)/am archive

LINKAGE += $(AM_HOME)/am/build/am-riscv32-nemu.a
```
3. 生成 dummy 的可执行文件需要 LINKAGE 中的文件，其中包含 AM 库。Make 为准备这个依赖，执行：
```makefile
$(MAKE) -s -C $(AM_HOME)/am archive
```
即进入 abstract-machine/am 目录，启动子 Make，构建 archive 目标。

4. 子 Make 读取 am/Makefile，确定库名和 AM 源文件列表，再读取公共 Makefile：
```makefile
NAME := am
SRCS = $(addprefix src/, $(AM_SRCS))
include $(AM_HOME)/Makefile
```
此时的 SRCS、OBJS 都属于 AM，不再是客户程序的 dummy.c、dummy.o。

5. 子 Make 的 MAKECMDGOALS 是 archive，因此进入归档分支。根据依赖关系，先将需要编译的 AM 源文件生成 .o,最终生成：
`abstract-machine/am/build/am-riscv32-nemu.a`

6. AM 子 Make 完成后，返回客户程序的构建。等 dummy.o、AM 库和 klib 库都准备好，再共同链接生成：`dummy-riscv32-nemu.elf`


#### 可执行文件的链接与内存布局

**Makefile 指定链接的输入和输出**

```makefile 
$(IMAGE).elf: $(LINKAGE) $(LDSCRIPTS)
    ...
    @$(LD) $(LDFLAGS) -o $@ --start-group $(LINKAGE) --end-group
```

LINKAGE: `dummy.o`， `am-riscv32-nemu.a`， `klib-riscv32-nemu.a`
输出: `dummy-riscv32-nemu.elf`
LD：`riscv-none-elf-ld`

```makefile 
LDSCRIPTS += $(AM_HOME)/scripts/linker.ld     # 按指定脚本安排内存布局

LDFLAGS   += --defsym=_pmem_start=0x80000000  # 定义物理内存的起始地址
             --defsym=_entry_offset=0x0       # 定义程序相对于物理内存起点的偏移量
             --gc-sections                    # 丢弃未使用的输入节
             -e _start                        # 指定ELF入口符号
```

**链接脚本安排内存布局**

链接脚本位于：`abstract-machine/scripts/linker.ld`: 

`. = _pmem_start + _entry_offset;` 设置程序的起始地址

```
低地址
0x80000000 ┌─────────────────────┐
           │ .text：启动及其他代码 
           ├─────────────────────┤
           │ .rodata：只读数据     
           ├─────────────────────┤
           │ .data：可写初始数据   
           ├─────────────────────┤
           │ .bss：初始为零的数据 
           ├─────────────────────┤
           │ 对齐填充             
_stack_top ├─────────────────────┤
           │ 栈空间，向低地址增长 
           │        ↓            
_stack_pointer ──────────────────┤
           │ 对齐后的堆可用区域     ← _heap_start
           │                     
PMEM_END   └─────────────────────┘
高地址
```

#### 程序的启动与退出

**执行起点**

- 链接脚本将`_start` 放在程序开头
- NEMU 将镜像加载到约定地址`_pmem_start = 0x80000000`, 同时设置初始PC到该地址
- 保证客户程序总能从`start.S` 开始


**Q:** 链接脚本如何将`_start` 放在程序开头？
**A:** start.S 中 `.section entry, "ax"` 表示后面的内容属于entry 的输入节; 而 `link.ld` 规定先放entry 节， 然后再放其他以.text 开头的代码节。

**设置栈指针, 进入客户程序**

```S
_start:
  mv s0, zero           # 将S0清零
  la sp, _stack_pointer # 将链接脚本确定的栈初始地址写入sp 
  call _trm_init        

```

**程序主体**

```c
void _trm_init() {
  int ret = main(mainargs); // 执行客户程序主体，返回值保存到ret 
  halt(ret);                // 调用halt()
}
```

**程序退出**

```c
void halt(int code) {
  nemu_trap(code);        // # define nemu_trap(code) asm volatile("mv a0, %0; ebreak" : :"r"(code))

  // should not reach here
  while (1);
}
```
结束码写入a0 -> 执行ebreak -> NEMU 执行ebreak, 读取a0 -> 设置NEMU_END, 根据结束码报告GOODTRAP/BADTRAP


