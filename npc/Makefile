# ============================================================
# NPC 仿真 Makefile
# 功能：Verilator 编译 + 仿真 + 波形生成
# 不包含 NVBoard 连接
# ============================================================

# -------------------- 基本配置 --------------------
TOPNAME = top
BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
BIN = $(BUILD_DIR)/$(TOPNAME)

# -------------------- Verilator 配置 --------------------
VERILATOR = verilator
VERILATOR_CFLAGS += -MMD --build -cc \
					-O3 --x-assign fast --x-initial fast --noassert

# -------------------- 波形配置 --------------------
WAVE_FILE = $(BUILD_DIR)/wave.vcd
LOG_FILE = $(BUILD_DIR)/sim.log
TRACE ?= 1

# 如果需要波形，添加 --trace
ifeq ($(TRACE), 1)
VERILATOR_CFLAGS += --trace
endif

# -------------------- 目录创建 --------------------
$(shell mkdir -p $(BUILD_DIR))

# -------------------- 源文件收集 --------------------
VSRCS = $(shell find $(abspath ./vsrc) -name "*.v" 2>/dev/null)
CSRCS = $(shell find $(abspath ./csrc) -name "*.c" -or -name "*.cc" -or -name "*.cpp" 2>/dev/null)

# -------------------- 编译选项 --------------------
INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""

# -------------------- 默认目标 --------------------
default: $(BIN)

# -------------------- Verilator 编译规则 --------------------
$(BIN): $(VSRCS) $(CSRCS)
	@echo "========================================="
	@echo "  编译 NPC"
	@echo "  顶层模块: $(TOPNAME)"
	@echo "  源文件:   $(words $(VSRCS)) 个 Verilog, $(words $(CSRCS)) 个 C++"
	@echo "========================================="
	@rm -rf $(OBJ_DIR)
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(TOPNAME) $(VSRCS) $(CSRCS) \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))
	@echo "编译完成: $(BIN)"

# -------------------- 仿真规则 --------------------
sim: $(BIN)
	@echo "========================================="
	@echo "  开始仿真"
	@echo "  可执行文件: $(BIN)"
	@echo "  日志文件:   $(LOG_FILE)"
	@echo "========================================="
	@$(BIN) 2>&1 | tee $(LOG_FILE)
	@echo ""
	@echo "仿真结束"
	@echo "日志保存在: $(LOG_FILE)"

# -------------------- 带波形的仿真 --------------------
sim-wave: $(BIN)
	@echo "========================================="
	@echo "  开始仿真（带波形）"
	@echo "  波形文件: $(WAVE_FILE)"
	@echo "========================================="
	@$(BIN) --wave=$(WAVE_FILE) 2>&1 | tee $(LOG_FILE)
	@echo ""
	@echo "仿真结束，波形已保存: $(WAVE_FILE)"
	@if [ -f $(WAVE_FILE) ]; then \
		echo "正在打开 GTKWave..."; \
		gtkwave $(WAVE_FILE) & \
	else \
		echo "警告: 未生成波形文件"; \
	fi

# -------------------- 运行规则（兼容） --------------------
run: sim

# -------------------- 查看波形 --------------------
wave:
	@if [ -f $(WAVE_FILE) ]; then \
		gtkwave $(WAVE_FILE) & \
	else \
		echo "错误: 波形文件不存在，请先运行 make sim-wave"; \
	fi

# -------------------- 查看日志 --------------------
log:
	@if [ -f $(LOG_FILE) ]; then \
		cat $(LOG_FILE); \
	else \
		echo "错误: 日志文件不存在，请先运行 make sim"; \
	fi

# -------------------- 清理规则 --------------------
clean:
	@echo "清理构建目录..."
	@rm -rf $(BUILD_DIR)
	@echo "清理完成"

# -------------------- 伪目标 --------------------
.PHONY: default sim sim-wave run wave log clean

# -------------------- 帮助信息 --------------------
help:
	@echo "NPC 仿真 Makefile"
	@echo ""
	@echo "可用目标:"
	@echo "  make            - 编译 NPC"
	@echo "  make sim        - 编译并仿真"
	@echo "  make sim-wave   - 编译并仿真（生成波形）"
	@echo "  make run        - 等同于 make sim"
	@echo "  make wave       - 查看波形（GTKWave）"
	@echo "  make log        - 查看仿真日志"
	@echo "  make clean      - 清理构建文件"
	@echo "  make help       - 显示帮助"
	@echo ""
	@echo "变量:"
	@echo "  TOPNAME=top     - 顶层模块名"
	@echo "  TRACE=1         - 启用波形追踪"