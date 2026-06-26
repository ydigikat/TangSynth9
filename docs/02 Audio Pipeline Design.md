# Audio Pipeline Design

The audio pipeline is pure RTL data plane, entirely independent of the MCU implementation.  It fans out into parallel voice pipelines before converging at a pipelined mixer and then onto  I2S output.  All timing is driven by the I2S peripheral.

## Top Level Structure

![alt text](assets/images/aud_overview.svg)

Each voice runs continously. Voices share the VRAM read port and summing mixer input but otherwise are entirely independent.

### Controller (VRAM Load Sequencer) 

The VRAM block is shared across all voices.  A round-robin sequencer loads voice parameters into local registers within each voice module once per control-block.  This happens when the VRCR register signals that new data is available.  VRAM is dual-ported with port-A used by the MCU and port-B by the audio pipeline.  Port-B is readonly.

- One VRAM word (32-bits) is read per cycle.
- Port A - CPU read write
- Port B - Pipeline readonly
- Data values are packed into words.  4x8 or 2x16, 1x32 bit values in each.
- Words are organised into blocks for each voice and individual words addressed as ```voice # x VRAM_VOICE_STRICE + word idx```. 
- Voices use their local registers during execution, VRAM is read only once per block.
- Each block comprises 8 words, a full load of 4 voices takes 32 clock cycles. There is ~512 cycles available between samples.

### Voice Pipeline

Each voice is itself an internally pipelined signal chain with a deterministic latency from input to output.  The pipeline is continous with no enable signal and runs at the sample rate.

![Voice Pipeline](assets/images/aud_voice.svg)


#### Stage 1: Oscillator (DCO)

OSC 1 and 2 run in parallel using independent phase accumulators.  Both are driven by their respective ```FCW``` (pitch) from VRAM. The pipeline depth is 2-3 cycles depending on waveform and BLEP antialiasing applied.

Each oscillator can produce a triangle, sawtooth, pulse or noise waveform selected by the ```osc_wave``` parameter.  Pulsewidth is set from ```osc_pw``` in the register file.

#### Stage 2: Filter (DCF)

The filter is a 4-pole zero-delay-feedback (ZDF) ladder filter. This is the stage with the deepest pipeline. ZDF filters require a feedback path which is resolved analytically rather than iteratively which makes them very suitable for a fixed-latency pipeline.

There are 2 possible implementations:

1. Unrolled.  All four poles are computed combinatorially within a single clock cycle using chained DSP blocks.  This is low-latency but could result in timing closure isues.
2. Pipelined.  Each pole is registered separately creating a pipeline depth of 4 cycles but guaranteing timing closure.  This is probably my preferred approach as it translates to more complex builds on more capable devices later.

I will explore both these options.

> **Tricky Stuff Ahead Alert**:      
> 
> Filter design in FPGA is new to me, prob best for me to prototype the filter using Python in floating point, then convert to fixed point. No access to MatLab now so can't use the fixed point tool.  
>   
>  ZDF filters are non-trivial and floating->fixed point present challeges as understanding all values is neccessary (more so than when using floats).  Normalisation will help here (SQ1.15).

Filter cutoff and resonance are set from the local register file. Modulation is applied by the pipeline at the block boundary: modulated_cutoff = filt_cutoff + (mod_source × filt_mod_depth).

#### Stage 3: Amplifier (DCA)

A single Q1.15 multiple of the filter output by the amp envelope.   This requires one 18x18 DSP multiplier at a cost of 1-2 cycles.  Envelope level comes from the register file and depth is applied at block load time.

### Modulation

Modulation is applied at the block boundary during the VRAM load sequence, not at sample rate.

The pipeline reads raw modulator levels (amp_eg_lvl, mod_eg_lvl, lfo_lvl) and static depth values from VRAM and calculates the modulated targets before storing them in the local register file.

The sample-rate signal chain then runs from the modulated values for the duration of the block.

This is a pipeline responsibility. The SOC writes raw modulator outputs only; it does not apply depth or compute modulated values.

### Pipelined Mixer

The mixer accumulates the voice outputs sequentially into a widened register, once voice per cycle.

- The accumulator width is VOICE_BITS + $clog2(VOICE_COUNT).
- Latency from first voice input to valid mix output = VOICE_COUNT cycles
- Output is scaled back for I2S using a right-shift without rounding.
- VOICE_COUNT is a parameter; the accumulator width, cycle count, and output shift all scale automatically

The mixer is triggered by the same cycle counter that tracks voice pipeline latency so it accumulates valid samples.

### I2S Output

The I2S peripheral drives all audio timing. 

Its sample request signal (req) is the heartbeat of the pipeline, it initiates the mix cycle, triggers the APCR update check, and ultimately clocks out the finished sample. 

The pipeline is a follower of I2S timing throughout.

### Key Values

These are the set of key values that control the audio pipeline.  Bracketed values are proposed values.

| Parameter | Description|
| --------- | ---------- |
| VOICE_COUNT | Number of parallel voices (6)|
| VOICE_BITS | Output word width (16) |
| VOICE_PIPELINE_LATENCY | Fixed cycles from input to valid voice output (TBD from filter implementation)|
| VRAM_VOICE_STRIDE| Words per voice slot in VRAM (8)|
| MIXER_ACC_BITS| VOICE_BITS + $clog2(VOICE_COUNT)|


---
> *Open Questions*  
>
> - Unrolled or pipelined filter?  
> - Anti-aliasing, PolyBLEP, worth the cycles?
> - Modulation timing,  cycle ordering of load, modulation multiply, and register writeback needs to be defined before sequencer FSM written
> - Latency calculation not known at this point - but well within constraints.














