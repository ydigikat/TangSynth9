# TangSynth9

TangSynth9 is a template SOC design providing a starting point for digital synthesiser projects.  

The SOC implementation comprises:

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

The Tang Nano 9K hosts the GW1NR-9 which is an FPGA + PSRAM (SIP)
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
| APCR   | Audio pipeline control register |
| TRACE  | Serial-tx module (used only for debug/trace) |
| MIDI   | Serial-rx module (used only for MIDI in) |
| GPO    | General purpose output (debug and on-board LED indicators)|

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

## Fixed Point Arithmetic Porting

| Signal | Range | Format | Notes |
| ------ | ----- | ------ | ----- |
| Phase Acc | 0-2^24 | uint32_t | Counter - no fraction |
| FCW  | 0-2^24 | uint32_t | Derivative of Phase Acc width |
| Control Rate Signals   | -1.0 : ~1.0 | SQ1.15 | Bipolar normalised |
| User Parameters | 0-2^7 | uint8_t | MIDI CC (7-bit) values |

Much of the code is ported from my MCU based template. This uses normalised floating point for all calculations as well as several math functions and employs a floating point unit.  The picorv32 is not suited to math, it has no floating point or math support at all.  

Rather than create fixed point math functions (or approximations) I use Python to generate precomputed lookup tables. 

The discrete values will change the characteristics of the synthesiser subtly however musically these are largely irrelevant, my floating point synths are all MIDI controlled so only calculate at 7-bit MIDI intervals so effectively discrete anyway.

## Parameter Mappings

Parameters are changed using MIDI CC messages, 7-bit range 0-127.  

These map either directly to a parameter value or are mapped via  LUT depending on the usage of the control.

```Non-generic``` LUTs are already precomputed for the MIDI values 0-127 so the parameter is just an index.  Attack, Filter Cutoff etc.  These are stored in the patch as an index.

The ```generic``` LUT is a precomputed log taper that works well for controls where finer control at the lower values is wanted.  The raw MIDI CC value is used as the index into the table:
```c
exp_value = midi_exp_curve_lut[cc_value]
``` 
Values used as multipliers are stored in patches as Q1.15 fixed point decimal values. 

Other values are stored unsigned except for the pitch offsets which are bipolar and need to be signed.

| Parameter | Stored Type | LUT | Notes |
| --------- | ---- | ----| ------|
| AMP_LEVEL | Q1.15 | generic | multiplier |
| AMP_MOD_SOURCE | uint8_t | - | enum value |
| AMP_MOD_DEPTH | Q1.15 | generic | multiplier |
| ENV_ATTACK | uint8_t | attack coeff| index | 
| ENV_SUSTAIN | Q1.15 | - | multipler. |
| ENV_DECAY |  uint8_t | decay + tco coeff | index | 
| ENV_RELEASE |  uint8_t | release + tco coeff | index | 
| ENV_NOTE_TRACL | bool | - | flag |
| ENV_VEL_TRACK | bool | - | flag |
| ENV_MODE | uint8_t | - | enum|
| OSC_WAVE | uint8_t | - | enum |
| OSC_LEVEL | Q1.15 | generic | muliplier |
| OSC_OCTAVE | int8_t | none | signed offset -2:2|
| OSC_SEMI | int8_t | none | signed offset -6:6|
| OSC_CENTS | int8_t | none | signed offset -50:50|
| OSC_PW | uint8_t | none | raw 7-bit - clamped to musical range|
| OSC_MOD_SOURCE | uint8_t | - | enum value |
| OSC_MOD_DEPTH | Q1.15 | generic | multiplier |
| LFO_RATE | Q1.15 | generic | multipler|
| LFO_MODE | int8_t | none | enum |
| GL_AMOUNT | Q1.15 | generic | glide amount, small amounts most useful  |
| GL_TIME | Q1.15 | generic | glide time, small amounts most useful | 
| FILT_CUTOFF | uint8_t | prewarped g-coeff | index | 
| FILT_RES | Q1.15 | generic | sensitive at high values |
| FILT_MODE | uint8_t | - | enum |
| FILT_MOD_SOURCE | uint8_t | - | enum value |
| FILT_MOD_DEPTH | Q1.15 | generic | multiplier |
| FILT_KEY_TRACK | bool | - | flag |
| SYNTH_VOICE_MODE | uint8_t | - | enum |
| SYNTH_PORTAMENTO_ON | bool | - | flag |
| SYNTH_PORTAMENTO_TIME | Q1.15 | generic | short times more useful |
| SYNTH_UNISON_DETUNE | Q1.15 | generic | multiplier|


