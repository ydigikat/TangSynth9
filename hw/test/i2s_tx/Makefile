# Module simulation Makefile for iverilog

# Module name - set this for each simulation directory
MOD := i2s_tx

# Directories
CURDIR := $(shell pwd)
BUILD_DIR := $(CURDIR)/build
RTL_DIR := $(CURDIR)/../../rtl/audio/

# Source files  
DUT := $(RTL_DIR)/$(MOD).sv
TB  := $(CURDIR)/$(MOD)_tb.sv
LOG := $(BUILD_DIR)/$(MOD).log

# Include any .svh files from RTL directory
RTL_INCLUDES := $(wildcard $(RTL_DIR)/*.svh)

# Output files
SIM_EXEC := $(BUILD_DIR)/$(MOD)_sim

# iverilog flags
IVERILOG_FLAGS := -g2012 -I$(RTL_DIR) -Wall -Wno-timescale

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Default target
.DEFAULT_GOAL := sim

# Compile and run simulation
sim: $(BUILD_DIR)
	@echo "Compiling $(MOD) testbench..."
	@iverilog $(IVERILOG_FLAGS) -o $(SIM_EXEC) $(DUT) $(TB) $(RTL_INCLUDES) 2>&1 | tee $(LOG)
	@echo "Running simulation..."
	@cd $(BUILD_DIR) && vvp $(notdir $(SIM_EXEC)) -fst 2>&1 | tee -a $(LOG)

# Check syntax only (no simulation)
check: $(BUILD_DIR)
	@echo "Checking syntax for $(MOD)..."
	@iverilog $(IVERILOG_FLAGS) -t null -o /dev/null $(DUT) $(TB) $(RTL_INCLUDES) 2>&1 | tee $(LOG)

# Launch GtkWave:
#   dump file: $(BUILD_DIR)/$(MOD).fst
#   save file: $(MOD).gtkw
trace:
	@echo "Loading GtkWave with $(BUILD_DIR)/$(MOD).fst ..."
	@gtkwave $(BUILD_DIR)/$(MOD)_tb.fst $(MOD)_tb.gtkw

# Clean build artifacts
clean:
	@rm -rf $(BUILD_DIR)

# Display help
help:
	@echo "Available targets:"
	@echo "  sim         - Compile and run simulation"
	@echo "  check       - Syntax check only (no simulation)"
	@echo "  trace       - Launch Gtkwave with dumpfile (and optional save file)"
	@echo "  clean       - Remove build directory"
	@echo "  help        - Display this help message"

.PHONY: sim sim-verbose check clean help trace