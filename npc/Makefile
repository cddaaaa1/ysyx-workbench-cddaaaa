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
NPC_SIM_VSRCS := $(VSRC_DIR)/npc_top.v $(VSRC_DIR)/dpic_mem.v
SOC_VSRCS   := $(shell find $(PERIP_DIR) \( -name "*.v" -o -name "*.sv" \)) $(ELABORATE_TOP)
HDRS        := $(wildcard $(VSRC_DIR)/include/*.vh) $(wildcard $(CSRC_DIR)/include/*.h)
SIM_CSRC    := $(filter-out $(CSRC_DIR)/emu_main.cpp $(CSRC_DIR)/minirvemu.cpp, \
                            $(wildcard $(CSRC_DIR)/*.cpp))
CDEFINES += -DTOP_NAME=V$(TOPNAME) 

ifeq ($(ARCH),minirv-npc)
  TOPNAME   := npc_top
  SIM_VSRCS := $(NPC_VSRCS)
  VDEFINES  := 
  CDEFINES  := 
  INC_FLAGS := -I$(VSRC_DIR)/include
  IMG       ?= $(abspath $(NPC_HOME)/../am-kernels/tests/cpu-tests/build/dummy-minirv-npc.bin)
  BOARD     :=
else ifeq ($(ARCH),minirv-ysyxsoc)
  TOPNAME   := SimTop
  SIM_VSRCS := $(filter-out $(NPC_SIM_VSRCS), $(NPC_VSRCS)) $(SOC_VSRCS)
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

# ---- iverilog 流程: 编出 .vpi 插件, 再让 vvp 在运行期加载 ----
OSS_INC ?= $(abspath $(dir $(shell which $(IVERILOG)))/../include/iverilog)
IV_DIR  := $(BUILD_DIR)
VPI_SO  := $(IV_DIR)/vpi.vpi
IV_VVP  := $(BUILD_DIR)/sim-iv.vvp

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
	$(IVERILOG) -g2012 -s $(TOPNAME) -o $@ $(if $(WAVE),-DDUMP) $(INC_FLAGS) $(SIM_VSRCS)

iv: $(VPI_SO) $(IV_VVP)

iv-run: iv
	@echo "ARCH=$(ARCH) IMG=$(IMG)"
	vvp -M$(IV_DIR) -m vpi $(IV_VVP) +img=$(IMG)

# ---- 网表仿真 (E8): 用综合网表替换 RTL 模块, 存储器仍由 npc_top 例化 ----
# 标准单元行为模型用 UDP 表描述, 系统 verilator(5.008) 不支持, 改用 oss-cad-suite 的
OSS_BIN     := $(patsubst %/,%,$(dir $(shell which $(IVERILOG))))
NET_VERILATOR ?= $(OSS_BIN)/verilator
PDK_HOME   ?= $(HOME)/.local/opt/icsprout55-pdk
CELL_VSRCS := $(wildcard $(PDK_HOME)/IP/STD_cell/ics55_LLSC_H7C_V1p10C100/*/verilog/*.v)
ECC_HOME   ?= $(HOME)/Projects/ecc
SYN_RUN    ?= netlist-sim
ECC_FLIST  := $(ECC_HOME)/npc/rtl/filelist.f
NETLIST_GZ ?= $(ECC_HOME)/npc/runs/$(SYN_RUN)/Synthesis_yosys/output/npc_Synthesis_sim.v.gz
NETLIST    := $(BUILD_DIR)/netlist/$(patsubst %.v.gz,%.v,$(notdir $(NETLIST_GZ)))
NET_VSRCS  := $(VSRC_DIR)/npc_top.v $(VSRC_DIR)/dpic_mem.v $(NETLIST) $(CELL_VSRCS)
NET_CSRC   := $(filter-out $(SRC_AUTO_BIND), $(SIM_CSRC))
NET_IMG    ?= $(abspath $(NPC_HOME)/../am-kernels/tests/cpu-tests/build/dummy-minirv-npc.bin)
NET_OBJ_DIR := $(BUILD_DIR)/obj_netlist
NET_BIN     := $(BUILD_DIR)/sim-netlist
NET_CDEFINES := -DNETLIST_SIM $(if $(WAVE),-DTRACE_ON,) \
                $(if $(MAXCYCLE),-DDEBUG_TIME -DMAX_CYCLE=$(MAXCYCLE),)
NET_IV_VVP  := $(BUILD_DIR)/sim-net-iv.vvp

# 综合一份 PC 复位值为 0x80000000 的网表: 去掉 YSYXSOC, 让 define.vh 走默认值
netlist-syn:
	@cp -f $(ECC_FLIST) $(ECC_FLIST).bak
	@sed -i '/^+define+YSYXSOC/d' $(ECC_FLIST)
	-@cd $(ECC_HOME) && ecc run --project npc --run-id $(SYN_RUN) --overwrite
	@mv -f $(ECC_FLIST).bak $(ECC_FLIST)

$(NETLIST): $(NETLIST_GZ)
	@mkdir -p $(dir $@)
	gunzip -c $< > $@

netlist: $(NETLIST)

netlist-verilator: $(NET_BIN)
	@echo "NETLIST IMG=$(NET_IMG)"
	$(NET_BIN) $(NET_IMG)

$(NET_BIN): $(NET_VSRCS) $(NET_CSRC) $(HDRS) $(NPC_HOME)/Makefile
	@mkdir -p $(BUILD_DIR)
	@rm -rf $(NET_OBJ_DIR)
	$(NET_VERILATOR) -MMD -O3 --x-assign fast --x-initial fast --noassert \
		--build -j $(shell nproc) -cc --exe --top-module npc_top \
		--timescale "1ns/1ns" --no-timing -Wno-fatal --autoflush \
		--trace \
		+define+__VERILATOR__ +define+functional +define+NETLIST_SIM \
		-I$(VSRC_DIR)/include \
		--Mdir $(NET_OBJ_DIR) -o $(abspath $@) \
		$(NET_VSRCS) $(NET_CSRC) \
		-CFLAGS "-I$(NPC_HOME) $(NET_CDEFINES)"

netlist-iverilog: $(VPI_SO) $(NET_IV_VVP)
	@echo "NETLIST IMG=$(NET_IMG)"
	vvp -M$(IV_DIR) -m vpi $(NET_IV_VVP) +img=$(NET_IMG)

$(NET_IV_VVP): $(NET_VSRCS) $(NPC_HOME)/iverilog_top.v $(VPI_SO)
	@mkdir -p $(BUILD_DIR)
	$(IVERILOG) -g2012 -s iverilog_top -Dfunctional -DNETLIST_SIM \
		-o $@ $(if $(WAVE),-DDUMP) -I$(VSRC_DIR)/include \
		$(NPC_HOME)/iverilog_top.v $(NET_VSRCS)

sim-clean:
	rm -rf $(OBJ_DIR) $(NET_OBJ_DIR) $(SIM_BIN) $(NET_BIN) $(IV_VVP) $(NET_IV_VVP) $(VCD)

clean:
	rm -rf $(BUILD_DIR) $(VCD)

.PHONY: default sim sim-run run sim-wave wave lint iv iv-run netlist netlist-syn \
        netlist-verilator netlist-iverilog sim-clean clean



