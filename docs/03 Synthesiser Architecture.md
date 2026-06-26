# Synthesiser Architecture

The template synthesiser is a fully functional, albeit simple, vintage analogue style design.  

It does not model any specific synthesiser nor does it use sophisticated DSP to obtain an analogue sound.  It serves mainly to demonstrate how the template may be used.

#### Oscillator (DCO)

Dual DCOs provide the basic waveforms typically found on early synthesisers with optional saturation/jitter to add a little analogue edge to them.  

DCOs can be pitched individually allowing detuning and provide pitch modulation from either the LFO (vibrato) or envelope generator.

- Sawtooth: band-limited (anti-aliased) using a Polynomial BLEP function. 
- Pulse: constructed from two saws with variable duty cycle. Since the saws are band-limited, so is the pulse.

- Triangle: little to no aliasing and using DCW yields no discernable difference so left without band-limiting.

- Noise: unfiltered white noise (even spectrum) from a pseudo-random number.

#### Envelope Generators

The envelope generators have an analogue style exponential segment curve based on Nigel Redmon's algorithm and are a standard 4-segment (ADSR) design.

The generator has 4 output modes:  

- Normal: Standard ADSR.
- Biased: Intended for pitch modulation.
- Inverted: The envelope is inverted (often used for filter effects)
- Inverted & Biased: A combination of two modes.


#### Filter (DCF)

The DCF (filter) is a zero delay feedback (ZDF) ladder implementation. 

#### Low frequency oscillator (LFO)

The LFO is based on a fixed matrix design that I first encountered on the Yamaha CS20M which generates all waveforms concurrently, the target of the LFO selects the type and depth it wants but all share the single LFO so run at the same rate.  The LFO offers several waveforms:

- Triangle
- Saw
- Ramp (reverse saw)
- Square (fixed 50% duty cycle)
- Sample and hold


#### Amplifier (DCA)

The DCA provides amplitude modulation (tremolo) but is otherwise just a simple level control.

