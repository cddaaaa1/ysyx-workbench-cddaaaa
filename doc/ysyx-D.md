# ysyx 学习记录 D 阶段

## 预处理

### 观察预处理结果
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

### 如何寻找头文件
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

### 用面向RISC-V架构的gcc进行预处理
 - `riscv64-linux-gnu-gcc -E a.c` 
 - `riscv64-linux-gnu-gcc ` 预先定义了 __riscv 使`printf("Hello RISC-V!\n");`被保留

### 对比gcc和riscv64-linux-gnu-gcc的预定义宏
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