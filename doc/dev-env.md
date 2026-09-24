# 换机器继续开发（NPC + ysyxSoC）

## 1. 仓库里有什么

主仓库 `git@github.com:cddaaaa1/ysyx-workbench-cddaaaa.git`（分支 `master`）只跟踪：

- `npc/`：RTL（`vsrc/`）、DPI/仿真 C++（`csrc/`，含 `csrc/include/npc.h`）、`Makefile`
- `abstract-machine/`：AM 源码与平台脚本（含自加的 `scripts/minirv-ysyxsoc.mk`）
- `E/nvboard/`、`ysyx.md`、`perf.md`、`init.sh`

`ysyx.md` 里有各阶段的进度记录，先看它。

## 2. 不在仓库里（Mac 上要自己恢复）

根 `.gitignore` 是「全忽略 + 白名单」，下面这些**没有进仓库**：

| 内容 | 怎么恢复 |
|---|---|
| `ysyxSoC/`（仿真的 SoC 顶层、`perip/`、`ready-to-run/minirv/{ElaborateTop.v,gen.sh,hello-minirv-ysyxsoc.bin}`） | `git clone https://github.com/OSCPU/ysyxSoC.git`，切到分支 `2607`（本机 HEAD：`a74c903 add ready-to-run for minirv`），放在工作区根目录下 |
| `ysyxSoC` 的本地补丁（必须，否则 CPU 接不上） | `ready-to-run/minirv/ElaborateTop.v` 中 `NPC core0 (` 改成 `ysyx_22040000 core0 (`，共 1 行 |
| `am-kernels/`（cpu-tests 等测试程序） | `git clone https://github.com/NJU-ProjectN/am-kernels.git`，切到分支 `ics2026` |
| AM 的预编译 klib（`abstract-machine/klib/build/klib-*.a`） | `cd abstract-machine/klib && make ARCH=minirv-ysyxsoc`（`**/build/` 被忽略） |
| 构建产物 `**/obj_dir/`、`**/build/`、镜像 `.bin/.elf` | 重新构建生成（见第 3 节） |
| 工具链（机器本地，不在任何仓库） | 见第 4 节 |

注意 `doc/` 本身也在忽略名单里（规则 `*`），往这里加文件要 `git add -f`。

## 3. 常用命令

```bash
# 编译 SoC 仿真（首次/改 RTL 后，几分钟）
cd npc && make sim

# 跑预置镜像
./build/sim-SimTop ../ysyxSoC/ready-to-run/minirv/hello-minirv-ysyxsoc.bin

# 带波形
make sim-wave IMG=../ysyxSoC/ready-to-run/minirv/hello-minirv-ysyxsoc.bin VCD=hello.vcd

# 跑 cpu-tests 的某个测试（如 dummy）——走 SoC 容器镜像流程
cd am-kernels/tests/cpu-tests && make ARCH=minirv-ysyxsoc ALL=dummy run
```

SoC 流程的镜像**不是裸 bin**：`ARCH=minirv-ysyxsoc` 会调 `ysyxSoC/ready-to-run/minirv/gen.sh`，把程序 ELF 嵌进 hello 镜像的 `0x5A200` 偏移处，由 flash 里的引导代码搬到 PSRAM `0x80000000` 再执行。
用 `ARCH=minirv-npc` 生成的裸 bin 烧进 flash 会直接卡死（没有引导代码、且链接地址是 `0x80000000`）。

## 4. 工具链

| 工具 | 本机版本 / 位置 | 说明 |
|---|---|---|
| verilator | 5.008（`/usr/bin`） | ≥5 即可 |
| riscv 裸机交叉 gcc | xPack `riscv-none-elf-gcc` 15.2.0，`~/.local/opt/xpack-riscv-none-elf-gcc-15.2.0-1` | AM 用 `abstract-machine/tools/minirv/` 里的 `minirv-gcc/g++` 包装脚本调用，需让前缀在 PATH 里 |
| python3、make、g++ | 系统自带 | `gen.sh`、`insert-arg.py` 需要 python3 |
| JDK 17 + mill/sbt | `~/.local/opt/temurin-17`、`sbt-1.13.0` | **只在需要重新生成 `ElaborateTop.v`** 时才要（`ysyxSoC/Makefile` 的 `verilog` 目标） |

代理：本机走内网代理 `http://172.38.11.182:20170`，换机器后不可用，拉 GitHub 要换自己的网络。

## 5. 踩过的坑

1. **`npc/Makefile` 的 verilator 必须带 `--build`**。少了它只生成 `obj_dir/` 的 C++ 和 makefile，不会编译链接，`make` 返回成功但你跑的仍是旧二进制（症状：改了 RTL 行为不变）。现在已加 `--build -j $(shell nproc)`。
2. 报 `No rule to make target .../csrc/pmem.h` 之类：`obj_dir/*.d` 里是删掉的头文件的旧依赖，`rm -rf obj_dir` 后重编。
3. **ebreak 约定：`a0 == 0` 才是 GOOD**（`halt(1)` 会打印 `HIT BAD TRAP: a0=1`，可参考 `archbench/result/303.cproc.log`）。RTL 里判反了就会把正常结束报成 BAD。
4. `$display` 的 `%s` 传 3 字节字符串（如 `"BAD"`）会带上一个 NUL，输出会多一个空格；用两个 `$display` 分支更稳。
5. Verilator 警告会被当成错误（Makefile 没有 `-Wno-fatal`）：`a0 ? ...` 这种「32 位当 1 位条件」会报 `WIDTHTRUNC`，要写显式比较 `a0 == 32'd0`。
6. `sim_retire` 这类 DPI 回调的符号名必须和 Verilog 的 `import "DPI-C" function ...` 完全一致，且 C++ 侧要 `extern "C"`。

## 6. 当前状态 / TODO

已完成：

- NPC 单机流程（`minirv-npc`）：riscv-tests、cpu-tests、hello / dummy 均通过
- ysyxSoC 流程：hello、dummy 跑通（`EBREAK: GOOD TRAP`）

待办：

- **性能异常**：SoC 上 17322 条指令用了 3,574,317 周期（≈206 周期/指令），取指/访存路径明显慢；单机流程是 0.5 IPC。优先查这里。
- 死代码清理：`npc/vsrc/ysyx_22040000.v` 里注释掉的旧 `ebreak <= ...` 块；`npc/csrc/dpi.cpp` 里没人读的 `g_retire_inst`。
- `npc/csrc/include/npc.h` 的 `extern SimState sim` 想再收敛一版，避免暴露全局状态。
- Difftest 在接入系统总线后还没更新。
- 批量跑 cpu-tests / archbench，过一遍 minirv 代码。
