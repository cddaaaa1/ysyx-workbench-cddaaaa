# ---- 工具 ----
VERILATOR := verilator

# ---- 路径 ----
NPC_HOME  := $(CURDIR)
SOC_HOME  ?= $(abspath $(NPC_HOME)/../ysyxSoC)
VSRC_DIR  := $(NPC_HOME)/vsrc
CSRC_DIR  := $(NPC_HOME)/csrc
BUILD_DIR := $(NPC_HOME)/build
OBJ_DIR   := $(NPC_HOME)/obj_dir

PERIP_DIR     := $(SOC_HOME)/perip
ELABORATE_TOP := $(SOC_HOME)/ready-to-run/minirv/ElaborateTop.v

# ---- 镜像 ----
IMG ?= $(SOC_HOME)/ready-to-run/minirv/hello-minirv-ysyxsoc.bin
VCD ?= dump.vcd

# ---- 仿真顶层 ----
SIM_TOP := SimTop
SIM_BIN := $(BUILD_DIR)/sim-$(SIM_TOP)

# ---- 源文件 ----
NPC_VSRCS := $(wildcard $(VSRC_DIR)/*.v)
SOC_VSRCS := $(shell find $(PERIP_DIR) \( -name "*.v" -o -name "*.sv" \)) $(ELABORATE_TOP)
SIM_VSRCS := $(filter-out $(VSRC_DIR)/sim_top.v, $(NPC_VSRCS)) $(SOC_VSRCS)
SIM_CSRC  := $(CSRC_DIR)/main.cpp $(CSRC_DIR)/dpi.cpp $(CSRC_DIR)/pmem.cpp $(CSRC_DIR)/minirvemu.cpp
VHDRS     := $(wildcard $(VSRC_DIR)/include/*.vh)

# ---- 编译选项 ----
INC_FLAGS := -I$(VSRC_DIR)/include -I$(PERIP_DIR)/spi/rtl -I$(PERIP_DIR)/uart16550/rtl

VERILATOR_CFLAGS  := -MMD -O3 --x-assign fast --x-initial fast --noassert --trace
VERILATOR_CFLAGS  += -cc --exe --top-module $(SIM_TOP)
VERILATOR_CFLAGS  += --timescale "1ns/1ns" --no-timing \
                     -D__VERILOG__ -DPDK_BEHAV -D__UART_TO_CONSOLE__
VERILATOR_CFLAGS  += $(INC_FLAGS)

# ---- 规则 ----
default: sim

sim: $(SIM_BIN)

$(SIM_BIN): $(SIM_VSRCS) $(SIM_CSRC) $(VHDRS)
	@mkdir -p $(BUILD_DIR)
	$(VERILATOR) $(VERILATOR_CFLAGS) --Mdir $(OBJ_DIR) -o $(abspath $(SIM_BIN)) \
		$(SIM_VSRCS) $(SIM_CSRC) -CFLAGS "-I$(NPC_HOME)"

sim-run: $(SIM_BIN)
	@$(SIM_BIN) $(IMG)

sim-wave: $(SIM_BIN)
	@NPC_TRACE=$(VCD) $(SIM_BIN) $(IMG)

sim-clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR) $(VCD)

clean: sim-clean

-include $(wildcard $(OBJ_DIR)/*.d)

.PHONY: default sim sim-run sim-wave sim-clean clean



