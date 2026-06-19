# TangSynth9

TangSynth9 is a template SOC design providing a starting point for digital synthesiser projects.

It is a SOC implementation comprising:

**SOC**

- SOC 
  - Yosys picorv32 soft-core CPU @ 24MHz
  - 16K RAM, 
  - IRQ support enabled
  - Hardware MUL/DIV/BARREL
  - MMIO modules.
- Audio pipeline
  - Audio control (voice fanout)
  - Voice sample signal chain (DSP)
  - Audio mixing (voice summing)
  - I2S output

The Tang Nano 9K hosts the GW1NR-9 which is a SIP (FPGA & PSRAM)
- 9K LUTs (approx)
- 468K BRAM
- 20 18x18 multipliers (DSP)
- 2 PLLs
- 608K User Flash
- 8MB PSRAM (on-die)
  
## SOC Architecture Overview

#### Hardware Architecture

![SOC Architecture](docs/assets/images/soc_architecture.svg)

The SOC divides the functionality into a control-plane, managed by the CPU, and a data plane which is a pure RTL pipeline.

The MCU handles the more complicated and slower rate logic, concerned with voice allocation, control rate signals (modulation), parameters and MIDI parsing.

The RTL data-plane handles the voice DSP and is independent from the MCU.

Communication uses the audio-interrupt from the I2S peripheral which signals to the MCU that data updates can be safely made to the voice configuration registers (timed between samples to avoid glitches).

Data updates are made using shared memory (VRAM).  The MCU audio interrupt ISR signals to the pipeline via the audio pipeline control register (APCR) to indicate when the shared data can be read.

This completely decouples the audio pipeline and MCU.

The SOC provides a set of design specific MMIO modules:

| Module | Purpose |
| ------ | ------- |
| SRAM   | CPU SRAM (BRAM) |
| VRAM   | Share Voice RAM |
| APCR | Audio pipeline control |
| TRACE  | Serial-tx module (used only for debug/trace) |
| MIDI   | Serial-rx module (used only for MIDI in) |
| GPO    | General purpose output (primarily on-board LED indicators)|

#### Software Architecture

![alt text](docs/assets/images/sw_architecture.svg)

| # | Notes | 
|---|-------|
| 1 | The bare-metal super loop handles all control rate calculations.|
| 2 | Param changes, MIDI events, and modulation signals are all latched into a set of internal structured|
| 3 | Incoming serial MIDI data interrupts the processor and the ISR stores it into a ring-buffer for processing by the super-loop.|
| 4 | The audio ISR indicates the point at which latched events can be written to voice ram (VRAM). |
| 5 | Once VRAM is updated, PCR status indicates it can be read by audio pipeline|
| 6 | THe VRAM holds current parameters for each voice|
| 7 | A mixer sums the voice outputs before transmitting via the I2S peripheral, the I2S peripheral signals when it is ready for the sample| 
| 8 | The I2S peripheral drives timing (sample request/audio IRQ) as well as output of samples|

## Timing

A single 24MHz clock times both the MCU and the audio pipeline.  This is a conservative value chosen to avoid timing challenges in the picorv32 which has some combinatorial chains with long propagation delays.  While modest, it is sufficient for audio processing and the control functions required.

The 24MHz clock is divided down to a 3MHz bit clock for the I2S peripheral.  This clock rate, the best the PLL can achieve, is not precisely that needed for a 48kHz Fs resulting instead in a 46.875kHz rate.

*Fs = BCLK / 64 = 3MHz / 64 = 46875 Hz*

This error is within tolerance for most DACs and should be corrected for by the source unit generator (FCW).

## Fixed Point Arithmetic Sizing

| Signal | Range | Format | Notes |
| ------ | ----- | ------ | ----- |
| Phase Acc | 0-2^24 | uint32_t | Counter - no fraction |
| FCW  | 0-Fs/2 | uint32_t | Derivative of Phase Acc width |
| Control Rate Signals   | 0.0-1.0 | Q1.15 | Unipolar normalised |
| User Parameters | 0-2^7 | uint8_t | MIDI CC (7-bit) values |

Much of the code is ported from my MCU based template. This uses normalised floating point for all calculations as well as several math functions and employs a floating point unit.  The picorv32 has no floating point or math support at all.  

Rather than create fixed point math functions (or approximations) I use Python to generate discrete lookup tables.  I sometimes use linear interpolation but continuous values are not musically important for the majority of synthesisers I build.

