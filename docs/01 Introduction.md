# Introduction

TangSynth9 is my SOC design that I use for FPGA synthesisers I build on the Tang Nano 9K board manufactured by Sipeed.

The Tang Nano 9K is a low-cost development board hosting a GW1NR-9 FPGA.

The GW1NR-9, manufactured by Gowin Semiconductor, is an SIP comprising - 
-  FPGA
   -  Around 9K LUTs
   -  468K BRAM
   -  20 DSP (18x18 multipliers)
   -  2 PLLs
   -  608K User Flash
- PSRAM 8MB

This is resource constrained but provides sufficient set of resources for many digital synthesiser projects. 

The low price-point makes it possible to sacrifice a board for permanent use in one-off projects.

## System-On-Chip (SOC)

TangSynth9 is a SOC.  It comprises a MCU supporting an audio pipeline. Both are implemented in RTL.

The MCU comprises the Yosys picorv32 soft-core CPU with a number of design specific memory-mapped (MMIO) modules to interface with peripherals and the audio pipeline.  

> This is not related to the MCU IP (also based on the picorv32) which Gowin provides in their toolset.
> 

#### MCU

The soft-core MCU is built around the picorv32 CPU using the native memory interface.  

- CPU : picorv32 @ 24MHz 
- Memory : 16K
- Options: Mult/Div/BarrelShift/IRQ support.
- MMIO: TRACE, GPO, PCSR, SRAM, VRAM, UART

The MMIO modules are specifically those needed for my designs and their purpose is covered later.

The 24MHz speed is modest but perfectly acceptable for the MCU's use as a control-plane in the synthesiser.  Control functions and rates can run at a far slower rate than sample generation.  There are some longish combinatorial chains in the CPU which, it my tests at least, start to approach a negative setup slack at around 50MHz.  











   

