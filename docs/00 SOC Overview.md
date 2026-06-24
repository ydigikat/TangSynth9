# TangSynth9 SOC Synthesiser

### Overview

The synthesiser is architecturally divided into 2 planes, an MCU built around a soft-core CPU and implementing the control plane functions, and an audio pipeline in pure RTL running the data plane.  Together these form the SOC.

![SOC diagram](assets/images/soc_architecture.svg)

Functionality is divided cleanly between the 2 planes.

The soft-core MCU handles voice lifecycle, parameters, MIDI parsing and generation of control rate signals.

The audio pipeline is purely concerned with the audio DSP, handling the DCO (oscillators), DCF (filter) and DCA (amplifier) modules.

Control rate signals run slower than sample rate, calculated every 32 samples, so are not sample accurate. This is musically inaudible.

Control data passes from the CPU to the audio pipeline via shared memory (VRAM) and carries data including modulator values and parameters needed by the audio pipeline.

Timing is driven by the audio pipline, from the I2S clocks, with an audio interrupt back to the CPU governing the control rate signals.  VRAM updates are timed between samples using this audio interrupt to avoid any potential audible glitches.

The combination of VRAM and interrupts completely decouples the control and data planes.  Either can be exercised and tested in isolation.

A single 24MHz clock times both the MCU and the audio pipeline.  This is a conservative value chosen to avoid timing challenges in the picorv32 which has some combinatorial chains with long propagation delays.  While modest, it is sufficient for the audio DSP and the MCU functions required.


### Hardware Design

The SOC is implemented using the Tang Nano 9K development board.  This hosts a Gowin GW1NR-9 FPGA.

The GW1NR-9 is a system-in-package, comprising both FPGA and PSRAM co-located on the same die (chiplet rather than shared silicon).

The FPGA is a relatively low-spec device with modest resources:

- 9K LUTs (approx)
- 468K BRAM
- 20 18x18 multipliers (DSP)
- 2 PLLs
- 608K User Flash
- 8MB PSRAM

This makes in an interesting challenge and an opportunity to explore some techniques that are useful when working with constrained devices.

The SOC chip implementation comprises:

- MCU
  - Yosys PicoRV32 soft-core CPU @ 24MHz
  - 16K BRAM
  - 1K VRAM
  - IRQ support enabled
  - Hardware MUL/DIV/BARREL
  - Custom MMIO modules
- Audio pipeline
  - Audio control 
  - Voice pipeline (4 parallel implementations)
  - Audio mixer
  - I2S peripheral

