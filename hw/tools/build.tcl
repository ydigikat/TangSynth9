#------------------------------------------------------------------------------
# (c) Jason Wilden 2025
#------------------------------------------------------------------------------

# Parse command line arguments
set do_program 0
set do_flash 0
set do_lint 0
set do_test 0

foreach arg $argv {
    switch -- $arg {
        "-program"  { set do_program 1 }
        "-flash"    { set do_flash 1 }
        "-lint"     { set do_lint 1 }
        "-test"     { set do_test 1 }        
        default    { puts "Warning: Unknown argument: $arg" }
    }
}

# Design
set DESIGN tangsynth9
set TOP_MODULE top

# Board
set DEVICE GW1NR-LV9QN88PC6/I5
set FAMILY GW1N-9C
set PACKAGE QN88P
set DEVICE_VER C
set BOARD tangnano9k

# Directories
set BASE_DIR [file dirname [file dirname [file normalize [file dirname [info script]]]]]
set RTL_DIR $BASE_DIR/hw/rtl
set CONSTRAINT_DIR $BASE_DIR/hw/constraints
set PROJ_DIR $BASE_DIR/hw/build
set IMPL_DIR $PROJ_DIR/$DESIGN/impl
set TEST_DIR $BASE_DIR/hw/test

# Bitstream output file
set BITSTREAM $IMPL_DIR/pnr/$DESIGN.fs

# This parses the HTML report files that Gowin EDA generates, it extracts some
# basic values that are useful to see during each build.  
proc parse_reports {} {
    global IMPL_DIR DESIGN BASE_DIR
    
    set main_report_file "${IMPL_DIR}/pnr/${DESIGN}.rpt.html"
    set timing_file "${IMPL_DIR}/pnr/${DESIGN}_tr_content.html"
    
    # Call Python script to extract data - easier than doing with TCL (at least for me)
    catch {exec python3 "${BASE_DIR}/hw/tools/parse_reports.py" "$main_report_file" "$timing_file"} report_output
    puts $report_output
    puts "==================================\n"
}

# Run Verilator linter on all SystemVerilog files
proc run_linter {rtl_files include_path} {
    puts "=================================================================="
    puts "## Running Verilator Lint"
    puts "=================================================================="
    
    set lint_failed 0
    foreach file $rtl_files {
        puts "## Linting: [file tail $file]"
        if {[catch {exec verilator --lint-only -Wall -Wno-DECLFILENAME -Wno-MODDUP -Wno-UNOPTFLAT -I$include_path $file} lint_output]} {
            puts $lint_output
            set lint_failed 1
        }
    }
    
    if {$lint_failed} {
        puts "\n## ERROR: Linting failed - build halted"
        exit 1
    }
    
    puts "## Linting passed"
    puts "==================================\n"
}

# Filter out excluded files from lint list
proc filter_lint_files {rtl_files exclude_list} {
    set filtered_files {}
    foreach file $rtl_files {
        set basename [file tail $file]
        if {[lsearch -exact $exclude_list $basename] == -1} {
            lappend filtered_files $file
        } else {
            puts "## Skipping lint: $basename (excluded)"
        }
    }
    return $filtered_files
}


# Run simulation tests
proc run_tests {test_dir} {
    puts "=================================================================="
    puts "## Running Simulation Tests"
    puts "=================================================================="
    
    set test_failed 0
    set test_count 0
    
    # Find all test directories containing Makefiles
    foreach test_subdir [glob -nocomplain -type d -directory $test_dir *] {
        set makefile "${test_subdir}/Makefile"
        if {[file exists $makefile]} {
            set test_name [file tail $test_subdir]
            incr test_count
            
            puts "## Running test: $test_name"
            set make_failed 0
            set output ""
            
            if {[catch {exec make -C $test_subdir 2>@1} output]} {
                set make_failed 1
            }
            
            puts $output
            
            # Check both exit code and output for FATAL or error
            if {$make_failed || [string match "*FATAL:*" $output] || [string match "*error:*" $output]} {
                puts "## ERROR: Test $test_name failed\n"
                set test_failed 1
            } else {
                puts "## Test $test_name passed\n"
            }
        }
    }
    
    if {$test_count == 0} {
        puts "## WARNING: No tests found in $test_dir"
    } elseif {$test_failed} {
        puts "## ERROR: One or more testbenches failed"
        exit 1
    } else {
        puts "## All $test_count testbenches passed"
    }
    
    puts "==================================\n"
}

# Diagnostics
puts "=================================================================="
puts "## Building $DESIGN ($TOP_MODULE)"
puts "## DEVICE: $DEVICE"
puts "## FAMILY: $FAMILY"
puts "## Base directory:  $BASE_DIR"
puts "## RTL directory: $RTL_DIR"
puts "## Project directory: $PROJ_DIR"
puts "## Bitstream: $BITSTREAM"
if {$do_test} { puts "## Testing: ENABLED" }
if {$do_lint} { puts "## Linting: ENABLED" }
if {$do_program} { puts "## Programming: MEMORY" }
if {$do_flash} { puts "## Programming: FLASH" }
if {!$do_program && !$do_flash} { puts "## Programming: SKIPPED" }

puts "=================================================================="


# RTL file list
set RTL_FILES [list \
    "$RTL_DIR/$TOP_MODULE.sv" \
    "$RTL_DIR/clock_gen.sv" \
    "$RTL_DIR/mcu/picorv32.v" \
    "$RTL_DIR/mcu/mcu.sv" \
    "$RTL_DIR/mcu/gpo.sv" \
    "$RTL_DIR/mcu/vrcr.sv" \
    "$RTL_DIR/mcu/serial_tx.sv" \
    "$RTL_DIR/mcu/buffer.sv" \
    "$RTL_DIR/mcu/trace.sv" \
    "$RTL_DIR/mcu/vram.sv" \
    "$RTL_DIR/mcu/sram.sv" \
    "$RTL_DIR/mcu/sram_block.sv" \
    "$RTL_DIR/mcu/vram_block.sv" \
    "$RTL_DIR/mcu/serial_rx.sv" \
    "$RTL_DIR/mcu/midi.sv" \
    "$RTL_DIR/audio/i2s_tx.sv" \
    "$RTL_DIR/audio/test_tone.sv" \
    "$RTL_DIR/audio/aud_pipeline.sv" 
]

# Files to exclude from linting (vendor primitives, etc)
set LINT_EXCLUDE [list \
    "picorv32.v"
]

# Optional test step - runs first if requested
if {$do_test} {
    run_tests $TEST_DIR
    # If only testing was requested, exit here
    if {!$do_lint && !$do_program && !$do_flash} {
        puts "## Completed."
        exit 0
    }
}

# Optional linting step
if {$do_lint} {
    set lint_files [filter_lint_files $RTL_FILES $LINT_EXCLUDE]
    run_linter $lint_files $RTL_DIR
}

# Clear any existing project
if {[file exists $PROJ_DIR]} {
    file delete -force $PROJ_DIR    
    file mkdir $PROJ_DIR
    puts "## Cleared existing project directory prior to new build"
}

# Create project 
create_project -name $DESIGN -dir $PROJ_DIR -pn $DEVICE -device_version $DEVICE_VER

# Global configuration
set_option -output_base_name $DESIGN
set_option -use_sspi_as_gpio 1

# Synthesis configuration
set_option -top_module $TOP_MODULE
set_option -verilog_std sysv2017 
set_option -synthesis_tool gowinsynthesis
set_option -include_path $RTL_DIR

# All warnings on
set_option -print_all_synthesis_warning 1

# PNR: No need to register io blocks, no high-speed signals and all module
# outputs are registered in fabric.
set_option -oreg_in_iob 0                

# Constraints
add_file "${CONSTRAINT_DIR}/${DESIGN}.cst"
add_file "${CONSTRAINT_DIR}/${DESIGN}.sdc"
   
# RTL 
foreach file $RTL_FILES {
    add_file $file
}

# Build
puts "## Building"
run all

# Cleaned output
catch {exec grep -v "picorv32.v" $IMPL_DIR/gwsynthesis/${DESIGN}.log} filtered_warns
puts "=================================================================="
puts $filtered_warns
puts "=================================================================="

parse_reports

# Optional programming step
if {$do_program || $do_flash} {
    puts "=================================================================="
    if {$do_program} {
        puts "## Writing bitstream to device memory"
        exec openFPGALoader -b $BOARD -m $BITSTREAM
    } elseif {$do_flash} {
        puts "## Writing bitstream to device flash"
        exec openFPGALoader -b $BOARD -f $BITSTREAM
    }
    puts "=================================================================="
} else {
    puts "## Programming skipped (use -program or -flash to enable)"
}





# All done.
puts "## Completed."