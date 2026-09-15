TOPNAME = top
VERILATOR = verilator
VERILATOR_CFLAGS += -MMD --build -cc \
				-O3 --x-assign fast --x-initial fast --noassert --trace

BUILD_DIR = ./build
OBJ_DIR = ./obj_dir

default: sim

# Pure Verilator simulation.
SIM_TOP = top
SIM_VSRCS = $(shell find $(abspath ./vsrc) -name "*.v")
SIM_CSRC = $(shell find $(abspath ./csrc) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
SIM_OBJ_DIR = $(OBJ_DIR)
SIM_BIN = ./build/sim-$(SIM_TOP)

sim: $(SIM_BIN)

$(SIM_BIN): $(SIM_VSRCS) $(SIM_CSRC)
	@rm -rf $(SIM_OBJ_DIR)
	@mkdir -p $(dir $(SIM_BIN))
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(SIM_TOP) $(SIM_VSRCS) \
		--Mdir $(SIM_OBJ_DIR) --exe $(SIM_CSRC) \
		-CFLAGS "-I$(CURDIR)" -o $(abspath $(SIM_BIN))

sim-run: $(SIM_BIN)
	@./$(SIM_BIN)

sim-clean:
	rm -rf $(SIM_OBJ_DIR) $(SIM_BIN) dump.vcd

.PHONY: default sim sim-run sim-clean
