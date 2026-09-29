# 环境配置变更记录

> 用途：① 记录跟环境 / 工具链 / 仓库结构有关的改动；② 换机器时快速对齐。
> 只写「环境怎么配」；学习进度和原理写在 `ysyx.md`，换机器的完整步骤写在 `doc/dev-env.md`。
> **新条目加在最上面**，格式：`### 日期 · 机器` + 改了什么 + 为什么 + 影响。

## 1. 机器一览

| 机器 | 系统 / 架构 | 用途 | 备注 |
|---|---|---|---|
| `DESKTOP-BSG4PAK` | WSL2 Ubuntu 26.04 / x86_64 | 主力 | 不能用 sudo；走内网代理 |
| Mac / UTM 虚拟机 | Linux ARM64 | 备用 | Verilator 在 `/opt/verilator/current` |

## 2. 每台机器各自设置（不进仓库）

| 项 | 本机（`DESKTOP-BSG4PAK`） | 说明 |
|---|---|---|
| `NVBOARD_HOME` | `<repo>/nvboard` | 写在 `~/.bashrc`；**改完必须 `source ~/.bashrc`**，否则 `npc/Makefile` 直接报「NVBOARD_HOME 未设置」 |
| `YSYX_VERILATOR_INCLUDE` | `/usr/local/share/verilator/include` | 只给 VS Code IntelliSense 用，编译不读它 |
| 代理 | `http://172.38.11.182:20170` | 换网络即失效，拉 GitHub 前先确认 |

## 3. 不变量（跨机器必须一致，否则出 bug）

- **UART16550 除数 = 13**：`abstract-machine/am/src/riscv/npc/trm.c` 里写 `13`，
  且 NVBoard 侧 `nvboard/src/uart.cpp` 必须是 `set_divisor(16 * 13)` = **208**。
  两边对不上 → 串口乱码，甚至 `term.cpp:107 Assertion 'ch < 128' failed`。
- `**/build/`、`**/obj_dir/` 换机器后一律重新生成，**不要跨架构拷二进制**。

## 4. 变更记录

### 2026-09-29 · DESKTOP-BSG4PAK
- **5 个子仓库改成 submodule**（`703ec05`）：`ysyxSoC` / `am-kernels` / `archbench` /
  `fceux-am` / `abstract-machine`，各自 fork 到 `cddaaaa1` 账号的 `ysyx` 分支。
  - 影响 1：**clone 必须带 `--recurse-submodules`**；已有工作区用 `git submodule update --init`。
  - 影响 2：改子仓库要「先子后父」提交两次，见 `dev-env.md`。
  - `abstract-machine` 的 `.git` 曾被 `init.sh`（`trace=true` 会 `rm -rf .git`）删掉，
    已用 `git init` + `fetch origin ics2026` + `reset --mixed FETCH_HEAD` 恢复。
  - `.gitignore` 去掉 `/ysyxSoC` `/am-kernels` `/archbench` `/fceux-am` 四行忽略规则。
- **`doc/` 纳入跟踪**：`.gitignore` 加 `!/doc/*`，以后往 `doc/` 加文件不用再 `git add -f`。
- **README 瘦身**：机器相关的 VS Code / Verilator 说明移进 `doc/`。

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
