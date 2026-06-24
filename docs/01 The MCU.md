# MCU Implementation

The MCU handles handles voice lifecycle, parameters, MIDI parsing and generation of control rate signals.

The MCU implements the picorv32 RISC processor together with a set of solution specific MMIO modules:

| Module | Purpose |
| ------ | ------- |
| SRAM   | CPU SRAM (BRAM) |
| VRAM   | Shared Voice RAM |
| VRCR   | VRAM control register |
| TRACE  | Serial-tx module (used for debug/trace) |
| MIDI   | Serial-rx module (used for MIDI in) |
| GPO    | On-board LEDs and misc IO|

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


## VRAM and Modulation Boundary

VRAM is the interface between the SOC control plane and the audio pipeline.

**The SOC is only a signal source, not a signal processor.**
It computes control-rate modulator outputs — amplitude envelope, modulation
envelope, and LFO level — and writes normalised Q1.15 values to VRAM once per
control block (~1ms at 46875Hz / 46 samples per block). It does not apply these
signals to their destinations. Voice lifecycle (note-on, note-off, stealing) and
static configuration (waveform select, FCW, filter parameters, mod depths) are
also written to VRAM, but only on change events, not every block.

**The pipeline owns all DSP, including modulation application.**
At the block boundary the pipeline reads raw modulator levels and depth
configuration from VRAM and computes modulated values — pitch, filter cutoff,
VCA level — before running the sample-rate signal chain. Depth scaling,
any non-linear shaping of modulator outputs, and any future per-sample
interpolation between block values are all pipeline responsibilities.

**Why the SOC does not pre-scale modulator outputs by depth.**
Pre-multiplying each modulator output by its destination depth on the SOC would
save a small number of pipeline multiply operations. This was considered and
rejected: it would move DSP signal routing decisions into firmware, preventing
the pipeline from independently changing how it applies modulation (interpolation,
waveshaping, alternate scaling) without requiring firmware changes. The pipeline's
hardware 18×18 DSP multipliers are the correct place for this work. DSP block
budget is preserved for the filter and oscillator anti-aliasing stages where it
is genuinely required at sample rate.


