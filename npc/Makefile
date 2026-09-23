TOPNAME = top
NXDC_FILES = constr/top.nxdc
INC_PATH ?=

VERILATOR = verilator
VERILATOR_CFLAGS += -MMD --build -cc \
				-O3 --x-assign fast --x-initial fast --noassert --trace
VERILATOR_CFLAGS += --timescale "1ns/1ns" --no-timing \
				-D__VERILOG__ -DPDK_BEHAV -D__UART_TO_CONSOLE__


BUILD_DIR = ./build
OBJ_DIR = ./obj_dir
BIN = $(BUILD_DIR)/$(TOPNAME)

default: $(BIN)

$(shell mkdir -p $(BUILD_DIR))

SRC_AUTO_BIND = $(abspath $(BUILD_DIR)/auto_bind.cpp)
$(SRC_AUTO_BIND): $(NXDC_FILES)
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $^ $@

VSRCS = $(shell find $(abspath ./vsrc) -name "*.v")
YSYXSOC_HOME ?= $(abspath ../ysyxSoC)
PERIP_DIR = $(YSYXSOC_HOME)/perip
PERIP_VSRCS = $(shell find $(PERIP_DIR) \( -name "*.v" -o -name "*.sv" \) 2>/dev/null)
VSRCS += $(PERIP_VSRCS)
ELABORATE_TOP = $(YSYXSOC_HOME)/ready-to-run/minirv/ElaborateTop.v
VSRCS += $(ELABORATE_TOP)
# 共享的 Verilog 头文件: 只参与依赖判断, 不作为源文件传给 verilator
VHDRS = $(shell find $(abspath ./vsrc/include) -name "*.vh")
# Verilog `include 的搜索路径, 让各模块能直接写 `include "define.vh"
VERILATOR_CFLAGS += -I$(abspath ./vsrc/include)
# perip 里 spi / uart16550 的源文件用 `include "xxx_defines.v" 引宏定义头文件
VERILATOR_CFLAGS += -I$(PERIP_DIR)/spi/rtl -I$(PERIP_DIR)/uart16550/rtl
CSRCS = $(shell find $(abspath ./csrc) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
CSRCS += $(SRC_AUTO_BIND)

include $(NVBOARD_HOME)/scripts/nvboard.mk

INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""

$(BIN): $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE) $(VHDRS)
	@rm -rf $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(TOPNAME) $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE) \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))

all: default

run: $(BIN)
	@$^

clean:
	rm -rf $(BUILD_DIR) $(OBJ_DIR)

.PHONY: default all clean run

# Pure Verilator simulation.
# 顶层改用 ysyxSoC 生成文件里的 SimTop: CPU(NPC) -> MemBridge -> AXI -> 各外设都在它下面
SIM_TOP = SimTop
SIM_TOP_SRC = $(ELABORATE_TOP)
# 旧的单 CPU 仿真壳 vsrc/sim_top.v 不参与本流程(它例化的 top + dpic_mem 只在旧流程里用)
SIM_VSRCS = $(filter-out $(abspath ./vsrc/sim_top.v), $(VSRCS))
SIM_CSRC = $(abspath ./csrc/main.cpp)
SIM_CSRC += $(abspath ./csrc/pmem.cpp)
SIM_CSRC += $(abspath ./csrc/minirvemu.cpp)
SIM_OBJ_DIR = $(OBJ_DIR)
SIM_BIN = ./build/sim-$(SIM_TOP)

sim: $(SIM_BIN)

$(SIM_BIN): $(SIM_VSRCS) $(SIM_CSRC) $(VHDRS)
	@rm -rf $(SIM_OBJ_DIR)
	@mkdir -p $(dir $(SIM_BIN))
	$(VERILATOR) $(VERILATOR_CFLAGS) --trace \
		--top-module $(SIM_TOP) $(SIM_VSRCS) \
		--Mdir $(SIM_OBJ_DIR) --exe $(SIM_CSRC) \
		-CFLAGS "-I$(CURDIR)" -o $(abspath $(SIM_BIN))

# IMG 可由命令行传入指定镜像路径; 为空时用 main.cpp 里的缺省值
# NPC_TRACE=dump.vcd ./build/sim-sim_top program/prog_add.bin
sim-run: $(SIM_BIN)
	@./$(SIM_BIN) $(IMG)

sim-clean:
	rm -rf $(SIM_OBJ_DIR) $(SIM_BIN) dump.vcd

.PHONY: sim sim-run sim-clean

# minirvEMU: 纯 C++ 的指令集模拟器, 不依赖 Verilator/NVBoard
EMU_INC = $(abspath ./csrc)
EMU_SRCS = $(abspath ./csrc/emu_main.cpp) $(abspath ./csrc/minirvemu.cpp)
EMU_HDRS = $(abspath ./csrc/minirvemu.h)
EMU_BIN = ./build/minirvemu

emu: $(EMU_BIN)

$(EMU_BIN): $(EMU_SRCS) $(EMU_HDRS)
	@mkdir -p $(dir $(EMU_BIN))
	$(CXX) -O2 -Wall -Wextra -I$(EMU_INC) -o $@ $(EMU_SRCS)

emu-run: $(EMU_BIN)
	@./$(EMU_BIN)

emu-clean:
	rm -rf $(EMU_BIN)

.PHONY: emu emu-run emu-clean
