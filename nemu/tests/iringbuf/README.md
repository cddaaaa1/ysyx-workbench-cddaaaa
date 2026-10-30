# iringbuf 测试

适用于当前 riscv32 NEMU，需开启 `CONFIG_ITRACE`。先循环执行超过 16 条指令，再触发错误。

在工作区根目录运行：

```sh
# 越界写入：panic → assert_fail_msg → iringbuf_dump
make -C nemu/tests/iringbuf ARCH=riscv32-nemu mainargs=oob run

# 当前未实现的 ecall：NEMU_ABORT → iringbuf_dump
make -C nemu/tests/iringbuf ARCH=riscv32-nemu mainargs=abort run
```

两种运行均预期报错，`make run` 返回失败属于预期结果。

终端应按执行顺序打印最近 16 条指令，与 `build/nemu-log.txt` 末尾的 16 条指令一致。越界写入发生在入队前，因此当前实现不会记录出错的写指令。

可从日志中提取最后 16 条指令作比较：

```sh
rg '^0x' nemu/tests/iringbuf/build/nemu-log.txt | tail -n 16
```
