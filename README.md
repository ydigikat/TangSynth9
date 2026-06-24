# TangSynth9

TangSynth9 is a template design for FPGA based digital synthesisers.  It also serves as an exploration of techniques, learning tool and test-bench for me.

This is a hobby project.  It will likely go for extended periods without updates when my professional work takes up my time.  While I am a commercial embedded engineer (with some 44 years experience), I do not apply the same rigour to my hobby projects as I would to a peer-reviewed commercial project, so expect to see things that make you wince.

### Documentation

[SOC Overview](<docs/00 SOC Overview.md>)  
[The MCU implementation](<docs/01 The MCU.md>)  
[The Audio Pipeline (RTL) implementation](<docs/02 The Audio Pipeline>)

### Current Status

24/06/2026 - Started building out the basic SOC framework.

### Folder Structure

> I work on Linux and my project layout and tooling is for that operating system. 

| Folder | Content |
| ------ | ------- |
| ```docs```   | Documentation files and assets.|
| ```hw/constraints```| CST/SDC constraint files |
| ```hw/rtl/audio``` | The audio pipeline RTL |
| ```hw/rtl/mcu``` | The MCU RTL |
| ```hw/rtl/``` | Common RTL and top module|
| ```test/*```  | Testbenches |
| ```tools```   | TCL build script and reports parser|
| ```sw/cmake```| C toolchain configurations|
| ```sw/source```| CMakeLists and main.c|
| ```sw/source/bsp```| Board support package |
| ```sw/source/synth```| The synthesiser control plane functions|
| ```sw/test``` | Unit tests|
| ```sw/tools```| Python scripts for LUT generation & 'ROM' loading|

The software build outputs go into the ```handoff``` folder.  These are split into 4 binary files for loading into the MCU during HDL synthesis.  There are also transient ```build``` folders created for HW and SW build artefacts.

### Tooling

The project is set up for work with Microsoft VSCode and includes build tasks to build and program the device.  A number of extensions are used and you will be recommended to install these by VSCode when you open the folder the first time.

I use the Gowin EDA tools for hardware implementation.  This is not open source but are free for use with non-commercial projects.  Note that the Gowin programmer does not work on Linux so OpenFPGALoader is used instead.  

The firmware is compiled using the standard GCC RISCV toolchain and assembler.

For Unit testing the native GCC compiler for Linux is used.

