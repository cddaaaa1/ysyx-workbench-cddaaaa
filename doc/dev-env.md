# 开发环境 / 换机器

> 只写「环境怎么配」和「改过什么」；学习进度、原理见 `ysyx.md`。
> 第 9 节的变更记录**新条目加在最上面**。

## 1. 机器一览

| 机器 | 系统 / 架构 | 用途 | 备注 |
|---|---|---|---|
| `DESKTOP-BSG4PAK` | WSL2 Ubuntu 26.04 / x86_64 | 主力 | 不能用 sudo；走内网代理 |
| Mac / UTM 虚拟机 | Linux ARM64 | 备用 | Verilator 在 `/opt/verilator/current` |

每台机器**各自设置、不进仓库**的：

| 项 | 本机（`DESKTOP-BSG4PAK`） | 说明 |
|---|---|---|
| `NVBOARD_HOME` | `<repo>/nvboard` | 写在 `~/.bashrc`；**改完必须 `source ~/.bashrc`**，否则 `npc/Makefile` 报「NVBOARD_HOME 未设置」 |
| `YSYX_VERILATOR_INCLUDE` | `/usr/local/share/verilator/include` | 只给 VS Code IntelliSense，编译不读 |
| 代理 | `http://172.38.11.182:20170` | 换网络即失效，拉 GitHub 前先确认 |

## 2. 仓库里有什么

主仓库 `https://github.com/cddaaaa1/ysyx-workbench-cddaaaa.git`（分支 `master`）跟踪：

- `npc/`：RTL（`vsrc/`）、DPI/仿真 C++（`csrc/`，含 `csrc/include/npc.h`）、`Makefile`
- `nvboard/`：NVBoard（含 `src/uart.cpp` 里的除数设置）
- `doc/`：本目录（环境说明 + 变更记录）
- `ysyx.md`、`perf.md`、`init.sh`、`.vscode/`
- 5 个**子模块**：`ysyxSoC/`、`am-kernels/`、`archbench/`、`fceux-am/`、`abstract-machine/`

`ysyx.md` 里有各阶段的进度记录，先看它。

### 5 个子模块

这几个外部仓库各自 fork 到 `cddaaaa1`，主仓库只记录指针，**clone 时必须带子模块**：

```bash
git clone --recurse-submodules https://github.com/cddaaaa1/ysyx-workbench-cddaaaa.git
# 已经 clone 过的：
git submodule update --init --recursive
```

| 子模块 | fork（分支 `ysyx`） |
|---|---|
| `ysyxSoC/` | https://github.com/cddaaaa1/ysyxSoC.git |
| `am-kernels/` | https://github.com/cddaaaa1/am-kernels.git |
| `archbench/` | https://github.com/cddaaaa1/archbench.git |
| `fceux-am/` | https://github.com/cddaaaa1/fceux-am.git |
| `abstract-machine/` | https://github.com/cddaaaa1/abstract-machine.git |

每个子模块里 `origin` 是上游（只读），`mine` 是自己的 fork。改了东西要**先提交子仓库、再提交主仓库**：

```bash
git -C ysyxSoC commit -am "..." && git -C ysyxSoC push
git add ysyxSoC && git commit -m "ysyxSoC: ..." && git push
```

克隆下来的子模块默认处于 **detached HEAD**，开改之前先 `git -C ysyxSoC checkout ysyx`。

`ysyxSoC` 原来要手工打的补丁（`ElaborateTop.v` 里 `NPC core0 (` → `ysyx_22040000 core0 (`）
已经进了 fork，不用再手工改。

### 不在任何仓库里（每台机器各自生成）

| 内容 | 怎么恢复 |
|---|---|
| AM 预编译 klib（`abstract-machine/klib/build/klib-*.a`） | `cd abstract-machine/klib && make ARCH=minirv-ysyxsoc`（`**/build/` 被忽略） |
| 构建产物 `**/obj_dir/`、`**/build/`、镜像 `.bin/.elf` | 重新构建生成（见第 3 节） |
| 工具链 | 见第 4 节 |

`riscv-tests/` 仍是普通忽略目录（没有自写改动），需要时自己 clone。
`doc/` 已纳入跟踪，直接 `git add doc/新文件.md` 即可。

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
| gh（GitHub CLI） | `~/.local/opt/gh/bin/gh` | 未登录；fork 时用 `GH_TOKEN` 从 `git credential fill` 拿 |

## 5. VS Code / 编辑器

共享的 `.vscode/c_cpp_properties.json` 有 `Linux ARM64` 和 `Linux x64` 两套配置，
用命令面板的 `C/C++: Select a Configuration` 选**实际编译所在 Linux 的架构**
（Remote SSH 时按远端选，不按本机界面所在的电脑）。

- 两套都用 `/usr/bin/g++`，适用于 Ubuntu（含 WSL/虚拟机）。
- 每台机器单独设 `YSYX_VERILATOR_INCLUDE` 为本机 Verilator 的 include 目录（里面有 `verilated.h`）：

  ```sh
  export YSYX_VERILATOR_INCLUDE=/usr/local/share/verilator/include
  ```

  它只给 IntelliSense 用；**必须被 VS Code 的远程扩展进程读到**，只在已打开的终端里 export 无效
  —— 改完重连，必要时通过 Remote SSH 重启远端 VS Code Server。
- `.vscode/c_cpp_properties.json` 已按当前目录结构更新（`nvboard/`、`npc/`），删掉了 `E/` 时代的旧路径。
- SSH 地址、桌面显示变量、机器专用的 GDB 设置放在仓库外的本机工作区或用户配置中。

## 6. 跨机不变量（不一致就出 bug）

- **UART16550 除数 = 13**：`abstract-machine/am/src/riscv/npc/trm.c` 里写 `13`，
  且 NVBoard 侧 `nvboard/src/uart.cpp` 必须是 `set_divisor(16 * 13)` = **208**。
  两边对不上 → 串口乱码，甚至 `term.cpp:107 Assertion 'ch < 128' failed`。
- `**/build/`、`**/obj_dir/` 换机器后一律重新生成，**不要跨架构拷二进制**。

## 7. 踩过的坑

1. **`npc/Makefile` 的 verilator 必须带 `--build`**。少了它只生成 `obj_dir/` 的 C++ 和 makefile，不会编译链接，`make` 返回成功但你跑的仍是旧二进制（症状：改了 RTL 行为不变）。现在已加 `--build -j $(shell nproc)`。
2. 报 `No rule to make target .../csrc/pmem.h` 之类：`obj_dir/*.d` 里是删掉的头文件的旧依赖，`rm -rf obj_dir` 后重编。
3. **ebreak 约定：`a0 == 0` 才是 GOOD**（`halt(1)` 会打印 `HIT BAD TRAP: a0=1`，可参考 `archbench/result/303.cproc.log`）。RTL 里判反了就会把正常结束报成 BAD。
4. `$display` 的 `%s` 传 3 字节字符串（如 `"BAD"`）会带上一个 NUL，输出会多一个空格；用两个 `$display` 分支更稳。
5. Verilator 警告会被当成错误（Makefile 没有 `-Wno-fatal`）：`a0 ? ...` 这种「32 位当 1 位条件」会报 `WIDTHTRUNC`，要写显式比较 `a0 == 32'd0`。
6. `sim_retire` 这类 DPI 回调的符号名必须和 Verilog 的 `import "DPI-C" function ...` 完全一致，且 C++ 侧要 `extern "C"`。
7. **子模块忘了推**：只推主仓库、没推子仓库时，别人 clone / `submodule update` 会报
   `fatal: reference is not a tree: <sha>`。永远「先子后父」。

## 8. 当前状态 / TODO

已完成：

- NPC 单机流程（`minirv-npc`）：riscv-tests、cpu-tests、hello / dummy 均通过
- ysyxSoC 流程：hello、dummy 跑通（`EBREAK: GOOD TRAP`）

待办：

- **性能异常**：SoC 上 17322 条指令用了 3,574,317 周期（≈206 周期/指令），取指/访存路径明显慢；单机流程是 0.5 IPC。优先查这里。
- 死代码清理：`npc/vsrc/ysyx_22040000.v` 里注释掉的旧 `ebreak <= ...` 块；`npc/csrc/dpi.cpp` 里没人读的 `g_retire_inst`。
- `npc/csrc/include/npc.h` 的 `extern SimState sim` 想再收敛一版，避免暴露全局状态。
- Difftest 在接入系统总线后还没更新。
- 批量跑 cpu-tests / archbench，过一遍 minirv 代码。

## 9. 变更记录

> **新条目加在最上面**，格式 `### 日期 · 机器` + 改了什么 + 影响。

### 2026-09-29 · DESKTOP-BSG4PAK
- **5 个子仓库改成 submodule**（`703ec05`）：`ysyxSoC` / `am-kernels` / `archbench` /
  `fceux-am` / `abstract-machine`，各自 fork 到 `cddaaaa1` 账号的 `ysyx` 分支。
  - 影响 1：**clone 必须带 `--recurse-submodules`**；已有工作区 `git submodule update --init`。
  - 影响 2：改子仓库要「先子后父」提交两次，见第 2 节。
  - `abstract-machine` 的 `.git` 曾被 `init.sh`（`trace=true` 会 `rm -rf .git`）删掉，
    已用 `git init` + `fetch origin ics2026` + `reset --mixed FETCH_HEAD` 恢复。
  - `.gitignore` 去掉 `/ysyxSoC` `/am-kernels` `/archbench` `/fceux-am` 四行忽略规则。
- **`doc/` 纳入跟踪**：`.gitignore` 加 `!/doc/*`，以后往 `doc/` 加文件不用再 `git add -f`。
- **README 瘦身**：机器相关的 VS Code / Verilator 说明移进本目录。
- `.vscode/c_cpp_properties.json` 改用当前路径，两套配置归一化。

### 2026-09-28 · DESKTOP-BSG4PAK
- **NVBoard 移到仓库根**（`0ebab10`）：`E/nvboard/` → `nvboard/` 并纳入主仓库跟踪。
  老机器 `~/.bashrc` 里若还指向 `E/nvboard`，必须改掉再 `source`。
- **删除 `E/`**（`fbcfd44`）：内容在历史 commit 中可查。
- **NVBoard 接入 NPC/SoC 仿真**（`818df29`）：`npc/constr/top.nxdc` 绑引脚，
  `npc/Makefile` include `nvboard.mk`，串口 TX 绑到 NVBoard 终端。
- **UART 除数最终定为 13**（`dfcb551`，commit 信息里写的是 14，以代码为准）。

### 2026-09-24 · DESKTOP-BSG4PAK
- 新增 `doc/dev-env.md`（`51add99`）：换机器继续开发的注意事项。

### 2026-09-23 · DESKTOP-BSG4PAK
- NPC 工程从 `E/minirv/` 移到仓库根 `npc/`（`f96eb33`）。
- `ysyxSoC` 以 `2607` 分支 clone 到 `ysyxSoC/`（`0439522`，现为 submodule）。

### 2026-09-18 · DESKTOP-BSG4PAK
- RISC-V 交叉工具链改用 xPack `riscv-none-elf-gcc` 15.2.0（装在 `~/.local/opt/`），
  因为 apt 的 `gcc-riscv64-linux-gnu` 没有 ilp32 multilib。
- JDK 17 + sbt 1.13.0 装在 `~/.local/opt/`（本机不能用 sudo）。
