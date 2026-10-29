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


### 通过批处理模式运行NEMU

```bash
cd nemu/
make ISA=riscv32 run ARGS="-b" IMG=../am-kernels/tests/cpu-tests/build/dummy-riscv32-nemu.bin
```

**-b参数传递**

Make 中设置 ARGS=-b → 将 -b 拼入 NEMU 启动命令 → NEMU 的 main() 通过 argv 接收参数 → parse_args() 识别 -b，调用 sdb_set_batch_mode() 设置批处理标志 → sdb_mainloop() 检查该标志，自动调用 cmd_c(NULL) 运行客户程序。

### 实现字符串处理函数 `string.c`

**strcmp**W
按照字典序，逐个比较两个以 `\0` 结尾的字符串。返回值只要求能表示小于、等于或大于零，不要求一定是 -1、0、1。

```c
int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s2) { // 两个字符都不是 '\0'，继续比较
    if (*s1 != *s2)
      return (unsigned char)*s1 - (unsigned char)*s2; // 首次不同处决定结果
    s1 ++;
    s2 ++;
  }
  return (unsigned char)*s1 - (unsigned char)*s2; // 处理相等或一个字符串是另一个的前缀
}
```

**strcat**
从 `dst` 字符串末尾的 `\0` 处开始追加 `src`，覆盖原来的结束符，再写入新的结束符。目标空间需要足够大，且源、目标区域不能重叠。

```c
char *strcat(char *dst, const char *src) {
  char *ret = dst; // 保存目标字符串起始地址
  while (*dst) dst ++; // 找到目标字符串的 '\0'
  while (*src != '\0') {
    *dst = *src;
    dst ++;
    src ++;
  }
  *dst = '\0';
  return ret; // 返回原始目标地址
}
```

**strcpy**
将 `src` 指向的 C 字符串复制到 `dst`，包括结尾的 `\0`。目标空间需要足够大，且源、目标区域不能重叠。

```c
char *strcpy(char *dst, const char *src) {
  char *ret = dst; // 保存目标起始地址
  while (*src != '\0') {
    *dst = *src;
    dst ++;
    src ++;
  }
  *dst = '\0'; // 复制字符串结束符
  return ret;
}
```

**strncpy**
最多复制 `n` 个字符。若在复制满 `n` 个字符前遇到 `\0`，会继续将目标剩余位置补为 `\0`；若前 `n` 个源字符中没有 `\0`，目标不会自动追加结束符。

```c
char *strncpy(char *dst, const char *src, size_t n) {
  char *ret = dst;
  for (; n > 0; n --) {
    if (*src != '\0') {
      *dst = *src;
      dst ++;
      src ++;
    } else {
      *dst = '\0'; // 源结束后，将目标剩余位置补零
      dst ++;
    }
  }
  return ret;
}
```

**strlen**
统计字符串中 `\0` 之前的字符数量，不把结束符计入长度。

```c
size_t strlen(const char *s) {
  size_t n = 0;
  while (*s) {
    n ++;
    s ++;
  }
  return n;
}
```

**strncmp**
最多比较前 `n` 个字符；遇到首个不同字符时返回它们的无符号字符值之差，遇到共同的 `\0` 或比较满 `n` 个字符则返回 `0`。因此 `n == 0` 时不读取字符串。

```c
int strncmp(const char *s1, const char *s2, size_t n) {
  for (; n > 0; n --) {
    if (*s1 != *s2) {
      return (unsigned char)*s1 - (unsigned char)*s2;
    }
    if (*s1 == '\0') {
      return 0; // 两个字符串在当前位置同时结束
    }
    s1 ++;
    s2 ++;
  }
  return 0; // 前 n 个字符相同
}
```

**strchr**
从头查找字符 `c` 第一次出现的位置；字符串结尾的 `\0` 也参与查找。找到时返回该位置的指针，未找到时返回 `NULL`。

```c
char *strchr(const char *s, int c) {
  do {
    if (*s == c) return (char *)s;
    if (*s == '\0') break;
    s ++;
  } while (1);
  return NULL;
}
```

注意：当前实现直接比较 `*s == c`，还没有先将 `c` 转换为 `char`；搜索高位字节或转换后为 `\0` 的参数时，结果可能不符合接口预期。

**strrchr**
查找字符 `c` 最后一次出现的位置，也会检查字符串结尾的 `\0`。当前实现先用 `strlen` 找到结束符，再从末尾向前查找。

```c
char *strrchr(const char *s, int c) {
  const char *p = s + strlen(s); // 指向结尾的 '\0'
  do {
    if (*p == c) return (char *)p;
    if (s == p) break; // 已经检查首字符
    p --;
  } while (1);
  return NULL;
}
```

它与 `strchr` 一样，当前尚未先将 `c` 转换为 `char`。

**memset**
从 `s` 开始，将连续 `n` 个字节设置为 `c` 转换成 `unsigned char` 后的值，并返回原始地址 `s`。

```c
void *memset(void *s, int c, size_t n) {
  void *ret = s; // 保存返回地址
  for (; n > 0; n --) {
    *(unsigned char *)s = c; // 写入一个字节
    s = (unsigned char *)s + 1; // 移动一个字节
  }
  return ret;
}
```

**memcpy**
从 `in` 复制 `n` 个字节到 `out`，包括值为零的字节；返回原始目标地址。源、目标区域不能重叠。

```c
void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *dst = out;
  const unsigned char *src = in; // 按字节访问，且不修改源数据
  for (; n > 0; n --) {
    *dst = *src;
    dst ++;
    src ++;
  }
  return out;
}
```

**memcmp**
按无符号字节值，比较两块内存区域的前 `n` 个字节，不受 `\0` 结束符影响。返回值的正负表示首个不同字节的大小关系；前 `n` 个字节都相同则返回 `0`。

```c
int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = s1;
  const unsigned char *p2 = s2; // void * 不能直接解引用或做指针加法
  for (; n > 0; n --) {
    if (*p1 != *p2) return *p1 - *p2;
    p1 ++;
    p2 ++;
  }
  return 0;
}
```

**memmove**
从 `src` 复制 `n` 个字节到 `dst`，即使两块区域重叠也保证结果正确，并返回原始目标地址。目标地址高于源地址时倒序复制，其他情况正序复制。

重叠的例子：

```c
char buf[] = "abcde";
memmove(buf + 1, buf, 4);
```

```text
下标：     0   1   2   3   4   5
原内容：   a   b   c   d   e  '\0'
源区域：  [a   b   c   d]
目标区域：    [b   c   d   e]
结果：     a   a   b   c   d  '\0'
```

如果使用简单的正序复制，第一次把下标 0 的 `a` 写到下标 1，就会覆盖还没读取的 `b`。`memmove` 因而从后往前复制：先复制下标 3，再依次复制下标 2、1、0。重叠区域上调用 `memcpy` 不符合其接口约定，不能依赖其结果。

```c
void *memmove(void *dst, const void *src, size_t n) {
  if (n == 0 || dst == src) return dst;

  const unsigned char *p1 = src;
  unsigned char *p2 = dst;
  if ((uintptr_t)dst > (uintptr_t)src) {
    p1 += n;
    p2 += n; // 两个指针先到区域末尾之后
    for (; n > 0; n --) {
      p1 --;
      p2 --; // 先退到最后一个有效字节
      *p2 = *p1;
    }
  } else {
    for (; n > 0; n --) {
      *p2 = *p1;
      p1 ++;
      p2 ++;
    }
  }
  return dst;
}
```


### 实现 sprintf

根据 `fmt` 和可变参数生成文本，写入 `out`，末尾添加 `\0`，返回字符数（不含结束符）。调用者需要保证目标空间足够。

```c
char out[64];
int len = sprintf(out, "name=%s, score=%d", "Tom", 95);
// out 为 "name=Tom, score=95"，len 为 18
```

调用关系：`sprintf` 管理可变参数 → `vsprintf` 解析格式 → `itoa` 转换整数。

**sprintf**
初始化可变参数列表，交给 `vsprintf` 处理，最后结束参数访问。

```c
int sprintf(char *out, const char *fmt, ...) {
  va_list ap; // 保存可变参数的读取状态
  va_start(ap, fmt); // 初始化，准备读取 fmt 后面的参数
  int len = vsprintf(out, fmt, ap); // 交给 vsprintf 解析格式并写入 out
  va_end(ap); // 结束可变参数访问
  return len; // 返回写入的字符数，不含结束符
}
```

**vsprintf**
逐个读取格式字符，处理普通文本、`%d`、`%s` 和 `%%`。`out` 指向下一个写入位置，`len` 记录总字符数。

```c
int vsprintf(char *out, const char *fmt, va_list ap) {
  int len = 0; // 累计输出字符数
  while (*fmt) {
    char ch = *fmt ++; // 先取当前格式字符，再移动格式指针
    if (ch != '%') { // 普通字符直接输出
      *out ++ = ch;
      len ++;
      continue; // 回到 while，避免普通字符进入格式转换
    }

    ch = *fmt ++; // 读取 % 后面的转换字符
    switch (ch) {
      case 'd': {
        int value = va_arg(ap, int); // 取出 %d 对应的整数值
        int written = itoa(out, value, 10); // 从当前输出位置写入十进制文本，返回其长度
        out += written; // 移到刚写入的结束符处，供后续内容覆盖
        len += written; // 累加数字文本长度，负号也计入
        break;
      }
      case 's': {
        const char *s = va_arg(ap, char *); // 取出 %s 对应的字符串指针
        while (*s) { // 只复制字符，结束符由函数末尾统一补上
          *out ++ = *s ++;
          len ++;
        }
        break;
      }
      case '%': // %% 输出一个 %，不取可变参数
        *out ++ = '%';
        len ++;
        break;
      default:
        panic("Unsupported format specifier"); // 当前只支持 %d、%s、%%
    }
  }
  *out = '\0'; // 为最终结果添加字符串结束符
  return len; // 不把结束符计入长度
}
```

**itoa**
将整数转换为文本并返回字符数。当前声明在 `klib.h`，实现在 `stdlib.c`；`vsprintf` 为 `%d` 传入 `radix = 10`。

```c
int itoa(char *str, int value , int radix){
  char digits[16]; // 暂存逆序数字字符，当前 32 位 int 的十进制转换足够使用
  int count = 0; // 临时数组中的数字数量
  int len = 0; // 已写入目标的字符数量
  unsigned int magnitude = (unsigned int)value; // 后续使用无符号运算处理数值大小

  if (value < 0) {
    str[len ++] = '-'; // 负数先写入负号
    magnitude = 0u - magnitude; // 得到数值大小，避免最小负整数直接取负溢出
  }

  do { // 至少执行一次，保证 0 也能生成字符
    digits[count ++] = '0' + magnitude % radix; // 取最低位，转成字符后逆序保存
    magnitude /= radix; // 去掉十进制最低位，当前与 radix = 10 配合
  } while (magnitude != 0);

  while (count > 0) {
    str[len ++] = digits[-- count]; // 先减少下标，再倒序取出数字写入目标
  }
  str[len] = '\0'; // 添加结束符，后续格式化输出可覆盖它
  return len; // 返回字符数，包含负号，不含结束符
}
```

以 `-120` 为例：先写入 `-`，临时数组依次保存 `0`、`2`、`1`，再倒序写入，得到 `"-120"`，返回长度 `4`。

当前 `itoa` 取余使用 `radix`，但除法固定为 `/ 10`，且尚未处理大于 9 的数字字符，因此这里仅按 `radix = 10` 使用。


