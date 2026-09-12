TOPNAME = top
NXDC_FILES = constr/top.nxdc
INC_PATH ?=

VERILATOR = verilator
VERILATOR_CFLAGS += -MMD --build -cc \
				-O3 --x-assign fast --x-initial fast --noassert --trace

BUILD_DIR = ./build
OBJ_DIR = ./obj_dir
BIN = $(BUILD_DIR)/$(TOPNAME)

default: $(BIN)

$(shell mkdir -p $(BUILD_DIR))

SRC_AUTO_BIND = $(abspath $(BUILD_DIR)/auto_bind.cpp)
$(SRC_AUTO_BIND): $(NXDC_FILES)
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $^ $@

VSRCS = $(shell find $(abspath ./vsrc) -name "*.v")
CSRCS = $(shell find $(abspath ./csrc) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
CSRCS += $(SRC_AUTO_BIND)

include $(NVBOARD_HOME)/scripts/nvboard.mk

INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""

$(BIN): $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE)
	@rm -rf $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(TOPNAME) $^ \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))

all: default

run: $(BIN)
	@$^

clean:
	rm -rf $(BUILD_DIR) $(OBJ_DIR)

.PHONY: default all clean run

# Pure Verilator simulation.
SIM_TOP = top
SIM_VSRCS = $(VSRCS)
SIM_CSRC = $(abspath ./csrc/main.cpp)
SIM_OBJ_DIR = $(OBJ_DIR)
SIM_BIN = ./build/sim-$(SIM_TOP)

sim: $(SIM_BIN)

$(SIM_BIN): $(SIM_VSRCS) $(SIM_CSRC)
	@rm -rf $(SIM_OBJ_DIR)
	@mkdir -p $(dir $(SIM_BIN))
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(SIM_TOP) $(SIM_VSRCS) \
		--Mdir $(SIM_OBJ_DIR) --exe $(SIM_CSRC) \
		-CFLAGS "-I$(CURDIR) -DNVBOARD_SIM" -o $(abspath $(SIM_BIN))

sim-run: $(SIM_BIN)
	@./$(SIM_BIN)

sim-clean:
	rm -rf $(SIM_OBJ_DIR) $(SIM_BIN) dump.vcd

.PHONY: sim sim-run sim-clean
