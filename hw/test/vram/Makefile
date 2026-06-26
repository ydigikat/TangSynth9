# Module simulation Makefile for iverilog
#
# This assumes that the module and testbench have the same name with
# the testbench adding a _tb suffix.
#
# The makefile brings these files and any headers in. If you need 
# additional modules add them to the ADDL variable.

# UUT module name (no extension)
MOD := vram

# Directories
CURDIR := $(shell pwd)
BUILD_DIR := $(CURDIR)/build
RTL_DIR := $(CURDIR)/../../rtl/mcu

# Source files (version controlled)
UUT := $(RTL_DIR)/$(MOD).sv
TB  := $(CURDIR)/$(MOD)_tb.sv
ADDL := $(RTL_DIR)/vram_block.sv
INCLUDE := $(wildcard $(RTL_DIR)/*.svh)
SAVE:= $(MOD)_tb.gtkw

# Output files (transient)
SIM := $(BUILD_DIR)/$(MOD)_tb.sim
LOG := $(BUILD_DIR)/$(MOD)_tb.log
DUMP:= $(BUILD_DIR)/$(MOD)_tb.fst

# iverilog flags
IVERILOG_FLAGS := -g2012 -I$(RTL_DIR) -Wall -Wno-timescale
VVP_FLAGS := -fst

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Default target
.DEFAULT_GOAL := sim

# Compile and run simulation using iverilog and vvp
sim: $(BUILD_DIR)
	@echo "Compiling $(MOD) testbench..."
	@iverilog $(IVERILOG_FLAGS) -o $(SIM) $(UUT) $(ADDL) $(TB) $(INCLUDE) 2>&1 | tee $(LOG)
	@echo "Running simulation..."
	@cd $(BUILD_DIR) && vvp $(notdir $(SIM)) $(VVP_FLAGS) 2>&1 | tee -a $(LOG)

# Check syntax only (no vvp simulation)
check: $(BUILD_DIR)
	@echo "Checking syntax for $(MOD)..."
	@iverilog $(IVERILOG_FLAGS) -t null -o /dev/null $(UUT) $(ADDL) $(TB) $(INCLUDE) 2>&1 | tee $(LOG)

# Launch GtkWave to view trace information, optional SAVE file
# can be created to store the set of signals to trace.
trace:
	@echo "Loading GtkWave with $(DUMP) ..."
	@gtkwave $(DUMP) $(SAVE)

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