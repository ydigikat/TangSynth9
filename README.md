# TangSynth9

TangSynth9 is a template design for FPGA based digital synthesisers on the Tang Nano 9K development board (GW1NR-9 FPGA).

It also serves as an exploration of some audio techniques, a learning and work-bench exploration tool for me.

>This is a hobby project.  It will likely go for extended periods without updates when my professional work takes up my time.  I am a commercial embedded engineer (with some 44 years experience) but I do not apply the same rigour to my hobby projects as I would to a peer-reviewed commercial project, so expect to see things that make you wince.

### Documentation

- [Overview](<docs/00 Overview.md>)
- [MCU Design](<docs/01 MCU Design.md>)
- [Audio Pipeline Design](<docs/02 Audio Pipeline Design.md>)
- [Synthesiser Architecture](<docs/03 Synthesiser Architecture.md>)

### Current Status

- 24/06/2026 - Started building out the basic SOC framework.
- 05/07/2026 - Audio pipeline, MIDI in and VRAM integration.

### Tooling

> This tooling configuration is for Linux (Ubuntu 22.04)

The project is set up for work with Microsoft VSCode and includes build tasks to build and program the device.  This is the simplest way to run the build tools.

| Task | Purpose |
| ---- | ------- |
| Build & Program  |  Builds both firmware & hardware. Programs bitstream to FPGA RAM|
| HW: Build | Builds hardware |
| HW: Program | Builds hardware and programs bitstream to RAM|
| HW: Flash | Builds hardware and flashes bitstream to FLASH|
| HW: Test | Runs hardware testbench verification|
| FW: Configure | Configures the CMake build for firmware |
| FW: Build | Builds the firmware |
| FW: Clean | Cleans the firmware build outputs |
| FW: Test | Run the firmware unit tests (Unity) |

#### Hardware Toolchain

The hardware toolchain is used to build and program/flash the FPGA bitstream.

| Tool  | Purpose |Notes |
| ----  | ----- | ---- |
| gw_sh | Build|Gowin EDA command line tool (TCL console) |
| openFPGALoader | Programming | Open source programmer. The Gowin programmer does not work on Linux|
| iverilog | Simulation |Hardware simulation/verification|
| gtkwave | Simulation| Hardware tracing/analysis|
| Verilator| Linting | Static HDL source analysis|

*The Gowin EDA tools are not open source but have a free license for non-commercial use.  Gowin generates a more optimal implementation than Yosys/Apicula open source tooling (for the present)*

The build is scripted using ```/hw/tools/build.tcl.``` and invoked using the command ```gw_sh /hw/tools/build.tcl -flags```

| Flag | Meaning |
| -----| ------- |
| -program | Programs the bitstream into the FPGA RAM|
| -flash | Programs the bitstream into the FPGA FLASH|
| -lint  | Lints all source using ```Verilator```|
| -test  | Runs testbench verifications |

#### Firmware Toolchain
The firmware tools are used to build the firmware binary and unit tests. Firmware should always be built before the hardware as it is handed off to the hardware build for inclusion in the bitstream.

| Tool  | Purpose |Notes |
| ----  | ----- | ---- |
| riscv32-unknown-elf-gcc | Cross-Compiler | Builds the RISCV32I firmware binary |
| x86_64-linux-gnu-gcc-11 | Native Compiler| Builds the Unity unit-tests|
| cmake | Build | Build configuration|
| ninja | Build | Build orchestration|
| bin_to_hex.py | Handoff |  Split & binary handoff to hardware|
| gen_luts.py | Code Gen | Generates various lookup tables for firmware|

The firmware build is scripted using ```CMake``` & ```Ninja``` and invoked using the command ```cmake```  

Unit tests are built with the Unity framework and invokved using the command ```cmake```

### VS Code Extensions (Optional)
These are entirely optional since the build works without them, however for a rich IDE type environment they are helpful. 

I do not provide an ```extensions.json``` file because there is no way to pin an extension to a version and (bizarrely) this is not considered an issue by the maintainers, despite it completely rendering the capability pointless - may as well remove it.  

```
<snarky> 
They are all too busy presumably stuffing more unwanted AI into the editor. 
</snarky>
```

Instead I recommend you run the individual commands to install them.  At the time of writing only the HDL extension needs to be pinned to a fixed version - it is being regularly updated and breaking changes are common.  I am pinned at the version that uses  ```svls (Dalance)``` rather than the more recent ```slang``` which is more capable but requires more configuration.

```sh

# HDL support
code --install-extension mshr-h.veriloghdl@1.27.4
code --install-extension dalance.svls-vscode
code --install-extension lramseyer.vaporview

# C/C++ & RISC ASM support
code --install-extension ms-vscode.cmake-tools
code --install-extension ms-vscode.cpptools-extension-pac
code --install-extension zixuanwang.linkerscript
code --install-extension trond-snekvik.gnu-mapfiles
code --install-extension davidegrayson.riscv-asm

# Python support
code --install-extension ms-python.python

# Others (TCL, Monitor etc)
code --install-extension rashwell.tcl
code --install-extension ms-vscode.vscode-serial-monitor
```

### AI Usage Disclosure

I <u>do not</u> use AI for architecting, designing, coding or documenting my synthesisers, firmware or RTL.

I do use AI to expand test coverage having written the basic tests myself (test-driven).


