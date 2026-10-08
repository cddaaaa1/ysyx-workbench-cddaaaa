VERILATOR := verilator
IVERILOG  := iverilog
SIM  ?= verilator
ARCH ?= minirv-ysyxsoc

NPC_HOME  := $(CURDIR)
SOC_HOME  ?= $(abspath $(NPC_HOME)/../ysyxSoC)
VSRC_DIR  := $(NPC_HOME)/vsrc
CSRC_DIR  := $(NPC_HOME)/csrc
BUILD_DIR := $(NPC_HOME)/build
OBJ_DIR := $(BUILD_DIR)/obj_dir

PERIP_DIR     := $(SOC_HOME)/perip
ELABORATE_TOP := $(SOC_HOME)/ready-to-run/minirv/ElaborateTop.v

# source
NPC_VSRCS   := $(wildcard $(VSRC_DIR)/*.v) $(wildcard $(VSRC_DIR)/libs/*.v)
SOC_VSRCS   := $(shell find $(PERIP_DIR) \( -name "*.v" -o -name "*.sv" \)) $(ELABORATE_TOP)
HDRS        := $(wildcard $(VSRC_DIR)/include/*.vh) $(wildcard $(CSRC_DIR)/include/*.h)
SIM_CSRC    := $(filter-out $(CSRC_DIR)/emu_main.cpp $(CSRC_DIR)/minirvemu.cpp, \
                            $(wildcard $(CSRC_DIR)/*.cpp))
CDEFINES += -DTOP_NAME=V$(TOPNAME) 

ifeq ($(ARCH),minirv-npc)
  TOPNAME   := ysyx_22040000
  SIM_VSRCS := $(NPC_VSRCS)
  VDEFINES  := 
  CDEFINES  := 
  INC_FLAGS := -I$(VSRC_DIR)/include
  IMG       ?= $(abspath $(NPC_HOME)/../am-kernels/tests/cpu-tests/build/dummy-minirv-npc.bin)
  BOARD     :=
else ifeq ($(ARCH),minirv-ysyxsoc)
  TOPNAME   := SimTop
  SIM_VSRCS := $(NPC_VSRCS) $(SOC_VSRCS)
  VDEFINES  := +define+YSYXSOC +define+__VERILOG__ +define+PDK_BEHAV +define+__UART_TO_CONSOLE__
  CDEFINES  := -DYSYXSOC
  INC_FLAGS := -I$(VSRC_DIR)/include -I$(PERIP_DIR)/spi/rtl -I$(PERIP_DIR)/uart16550/rtl
  IMG       ?= $(SOC_HOME)/ready-to-run/minirv/hello-minirv-ysyxsoc.bin
  BOARD     ?= 1                                 
else
  $(error ARCH 只支持 minirv-npc / minirv-ysyxsoc, 当前是 "$(ARCH)")
endif

ifeq ($(SIM),iverilog)
  TOPNAME   := iverilog_top
  SIM_VSRCS += $(NPC_HOME)/iverilog_top.v
else ifneq ($(SIM),verilator)
  $(error SIM 只支持 verilator / iverilog, 当前是 "$(SIM)")
endif

SIM_BIN := $(BUILD_DIR)/sim-$(TOPNAME)

ifneq ($(BOARD),)
ifeq ($(NVBOARD_HOME),)
$(error NVBOARD_HOME 未设置: 先 `source ~/.bashrc` 或 export NVBOARD_HOME=...)
endif
NXDC_FILES    := $(NPC_HOME)/constr/top.nxdc
SRC_AUTO_BIND := $(BUILD_DIR)/auto_bind.cpp
NVBOARD_CFLAGS  := -I$(NVBOARD_HOME)/usr/include
include $(NVBOARD_HOME)/scripts/nvboard.mk  
CDEFINES += -DNVBOARD
SIM_CSRC  += $(SRC_AUTO_BIND)

$(SRC_AUTO_BIND): $(NXDC_FILES)
	@mkdir -p $(dir $@)
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $^ $@
endif

VCD ?= dump.vcd

VERILATOR_CFLAGS  := -MMD -O3 --x-assign fast --x-initial fast --noassert --trace
VERILATOR_CFLAGS  += --build -j $(shell nproc) -cc --exe --top-module $(TOPNAME)
VERILATOR_CFLAGS  += --timescale "1ns/1ns" --no-timing -Wno-fatal
VERILATOR_CFLAGS  += --autoflush
VERILATOR_CFLAGS  += $(INC_FLAGS) $(VDEFINES)

ifneq ($(WAVE),)
CDEFINES += -DTRACE_ON
endif

ifneq ($(MAXCYCLE), )
CXXFLAGS += -DDEBUG_TIME -DMAX_CYCLE=$(MAXCYCLE)
endif

# rules
ifeq ($(SIM),iverilog)
default: iv-run
run:     iv-run
sim:     iv
sim-run: iv-run
else
default: sim
run:     sim-run

sim: $(SIM_BIN)

$(SIM_BIN): $(SIM_VSRCS) $(SIM_CSRC) $(HDRS) $(NVBOARD_ARCHIVE) $(NPC_HOME)/Makefile
	@mkdir -p $(BUILD_DIR)
	@rm -rf $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_CFLAGS) --Mdir $(OBJ_DIR) -o $(abspath $(SIM_BIN)) \
		$(SIM_VSRCS) $(SIM_CSRC) $(NVBOARD_ARCHIVE) \
		-CFLAGS "-I$(NPC_HOME) $(NVBOARD_CFLAGS) $(CDEFINES) $(CXXFLAGS)" \
		$(if $(strip $(LDFLAGS)),-LDFLAGS "$(LDFLAGS)")

sim-run: $(SIM_BIN)
	@echo "ARCH=$(ARCH) TOP=$(TOPNAME) IMG=$(IMG)"
	@$(SIM_BIN) $(IMG)

sim-wave:
	@$(MAKE) -s sim WAVE=1
	@$(SIM_BIN) $(IMG)
endif

wave:
	gtkwave $(VCD) &

lint:
	$(VERILATOR) --lint-only -Wall -Wno-DECLFILENAME $(INC_FLAGS) $(VDEFINES) \
		--top-module ysyx_22040000 $(NPC_VSRCS)

# ---- iverilog 流程 (SIM=iverilog): 编出 .vpi 插件, 再让 vvp 在运行期加载 ----
ifeq ($(SIM),iverilog)
OSS_INC ?= $(abspath $(dir $(shell which $(IVERILOG)))/../include/iverilog)
IV_DIR  := $(BUILD_DIR)
VPI_SO  := $(IV_DIR)/vpi.vpi
IV_VVP  := $(IV_DIR)/sim-iv.vvp

$(IV_DIR)/vpi.o: $(CSRC_DIR)/vpi.c $(CSRC_DIR)/include/pmem.h
	@mkdir -p $(IV_DIR)
	$(CC) -O2 -fPIC -I$(OSS_INC) -I$(CSRC_DIR) -c $< -o $@

$(IV_DIR)/vpi_pmem.o: $(CSRC_DIR)/pmem.cpp $(CSRC_DIR)/include/pmem.h
	@mkdir -p $(IV_DIR)
	$(CXX) -O2 -fPIC -c $< -o $@

$(VPI_SO): $(IV_DIR)/vpi.o $(IV_DIR)/vpi_pmem.o
	$(CXX) -shared -o $@ $^

$(IV_VVP): $(SIM_VSRCS) $(HDRS) $(VPI_SO)
	@mkdir -p $(IV_DIR)
	$(IVERILOG) -g2012 -o $@ $(if $(WAVE),-DDUMP) $(INC_FLAGS) $(SIM_VSRCS)

iv: $(VPI_SO) $(IV_VVP)

iv-run: iv
	@echo "ARCH=$(ARCH) IMG=$(IMG)"
	vvp -M$(IV_DIR) -m vpi $(IV_VVP) +img=$(IMG)
endif

sim-clean:
	rm -rf $(OBJ_DIR) $(SIM_BIN) $(VCD)

clean:
	rm -rf $(BUILD_DIR) $(VCD)

.PHONY: default sim sim-run run sim-wave wave lint iv iv-run sim-clean clean



