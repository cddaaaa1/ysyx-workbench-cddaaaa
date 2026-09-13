# "一生一芯"工程项目

这是"一生一芯"的工程项目. 通过运行
```bash
bash init.sh subproject-name
```
进行初始化, 具体请参考[实验讲义][lecture note].

[lecture note]: https://ysyx.oscc.cc/docs/


## ARM64 / x64 Linux 的 VS Code 配置

共享的 `.vscode/c_cpp_properties.json` 包含 `Linux ARM64` 和 `Linux x64` 两套配置。
在命令面板执行 `C/C++: Select a Configuration`，选择实际编译所在 Linux 的架构。
Remote SSH 连接时依据远端 Linux 的架构选择，不依据运行 VS Code 界面的电脑。
选择配置不需要反复修改 JSON。此设置用于 C/C++ IntelliSense，实际构建仍由 Makefile 和编译器决定。

两套配置默认使用 `/usr/bin/g++`，适用于 Ubuntu Linux（含相应架构的虚拟机或 WSL）。
每台机器单独设置 `YSYX_VERILATOR_INCLUDE` 为 Verilator 头文件所在目录，目录中应有 `verilated.h`。
例如在本机 Linux 的 `~/.profile` 或由它加载的工具配置中设置：

```sh
# 根据本机的 Verilator 安装位置填写，以下只是一个路径示例。
export YSYX_VERILATOR_INCLUDE=/usr/local/share/verilator/include
```

本次 UTM 环境使用 `/opt/verilator/current/share/verilator/include`，变量保存在仓库外的
`~/.config/ysyx/tools.sh` 中，已由 `~/.profile` 加载。
可运行 `verilator -V` 查看安装信息，用 `test -f "$YSYX_VERILATOR_INCLUDE/verilated.h"` 检查路径。
该变量必须被 VS Code 的远程扩展进程读取；仅在已打开的终端里 export 不会更新扩展进程。
首次设置后重新连接；若仍显示变量未定义，在保存工作后通过 Remote SSH 命令重启远程 VS Code Server，再连接。

源码、Makefile 和这两套编辑器配置由 Git 同步。`build/`、`obj_dir/` 在两台机器上分别生成，
不要跨 ARM64 和 x64 复制二进制编译目录。Verilator 版本尽量保持一致。
SSH 地址、桌面显示变量及机器专用的 GDB 设置放在仓库外的本机工作区或用户配置中。
