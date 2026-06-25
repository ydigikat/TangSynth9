# TangSynth9 SOC Synthesiser

### Overview

TangSynth9 is a complete but relatively simple 6-voice digital synthesiser with typical analogue style waveforms, filter and envelope generators.  

For full details see: [Synthesiser Architecture](<03 Synthesiser Architecture.md>)

Waveforms are directly calculated and antialiasing provided for the sawtooth wave using a Polynomial BLEP approach.  Since the pulse wave is generated using the 'sum of two saws' approach so anti-aliased as a result.  

The triangle wave contains few unwanted harmonics, a test of DCW antialiasing showed no discernable audible difference so no antialiasing is used for this.

The synthesiser design is architecturally divided into 2 planes, an MCU built around a soft-core CPU and implementing the control plane functions, and an audio pipeline in pure RTL running the data plane.  Together these form the SOC.

![SOC diagram](assets/images/soc_architecture.svg)

Functionality is divided cleanly between the 2 planes.

The MCU handles voice lifecycle, parameters, MIDI parsing and generation of control rate signals.

The Audio Pipeline is concerned only with audio DSP, handling the DCO (oscillators), DCF (filter) and DCA (amplifier) modules.

Control rate signals run slower than sample rate are stepped rather than sample accurate.  The control block length (stepping) is 46 samples which is an update every ~1ms which is musically inaudble.

Control data passes from the CPU to the audio pipeline via shared memory (VRAM) with access controlled by the VRCR (Voice RAM Control Register). This memory holds modulator values and parameters managed by the MCU and required by the audio pipeline during DSP operations.

Timing is driven by the audio pipeline, from the I2S word-select clock, with an audio interrupt back to the CPU governing the control rate blocks.  VRAM updates are also timed to be read by the audio pipeline between samples to avoid any potential audible glitches.

The combination of VRAM and interrupts completely decouples the control and data planes, each can be developed/tested in isolation.

A single 24MHz clock times both the MCU and the audio pipeline.  This is a conservative value chosen to avoid timing challenges in the picorv32 which exhibits some combinatorial chains with long propagation delays on the GW1NR-9.  It also makes timing closure simpler and helps avoid PNR congestion.  It is sufficient for the audio DSP and the MCU functions required.


### Hardware Design

The SOC is implemented on the [Tang Nano 9K](https://wiki.sipeed.com/hardware/en/tang/Tang-Nano-9K/Nano-9K.html) development board which hosts a [Gowin GW1NR-9 FPGA](https://cdn.gowinsemi.com.cn/DS117E.pdf).

The GW1NR-9 is a system-in-package, comprising both FPGA and PSRAM co-located on the same die (chiplet rather than shared silicon).

The FPGA is a relatively low-spec device with modest resources:

- 9K LUTs (approx)
- 468K BRAM
- 20 18x18 multipliers (DSP)
- 2 PLLs
- 608K User Flash
- 8MB PSRAM

This makes the design an interesting challenge and an opportunity to explore some techniques that are useful when working with constrained devices.

The SOC chip implementation comprises:

- [MCU](<01 MCU Design.md>)
  - [Yosys PicoRV32](https://github.com/YosysHQ/picorv32) CPU @ 24MHz
  - 16K BRAM
  - 1K VRAM
  - IRQ support enabled
  - Hardware MUL/DIV/BARREL SHIFTER enabled
  - Custom MMIO modules
- [Audio Pipeline](<02 Audio Pipeline Design.md>)
  - Controller @ 24MHz
  - Voice pipeline (6 parallel implementations)
  - Summing mixer
  - I2S TX controller (48kHz, 16-bit, Philips standard)
