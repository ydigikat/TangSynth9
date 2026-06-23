#!/usr/bin/env python3
# ------------------------------------------------------------------------------
# (c) Jason Wilden, 2026
# ------------------------------------------------------------------------------
#
# Generates the various lookup tables used by the SOC to avoid the need
# for calculations or exp/log apporximations.
#
# NOTE: These compensate for the inaccurate Fs from limitated scalers in the PLL.  
# ------------------------------------------------------------------------------

import math

# Common constants
SAMPLE_RATE      = 46875.0       # Actual Fs 
AUDIO_BLOCK_SIZE = 32            # Samples per audio block
Q15_ONE = 32767                  # Max unsigned
Q15_MIN_SIGNED = -32768          # Min signed
Q15_MAX_SIGNED = 32767           # Max signed

# Envelope generator constants
ATTACK_TCO  = math.exp(-1.5)     # Time constant overshoots
DECAY_TCO   = math.exp(-4.95)
RELEASE_TCO = DECAY_TCO
ENV_TIME_MIN_MS      = 1.0       # Min ADSR segment time
ENV_TIME_MAX_MS      = 8000.0    # Max ADSR segment time
NUM_STEPS = 128                  # Max MIDI (7-bit) value

# Pitch table constants
PHASE_ACC_BITS = 24              # Phase accumulator width (0-2^24, per README sizing table)
PHASE_ACC_MAX  = 1 << PHASE_ACC_BITS
A4_MIDI_NOTE   = 69               # MIDI note number for A4
A4_FREQ_HZ     = 440.0            # Concert pitch reference

# Generic exp curve constants
EXPONENT = 2.5

# LFO increment constants
LFO_RATE_MAX_HZ  = 20.0
PHASE_ACC_16_MAX = 65536


# MIDI value to time
def midi_to_ms(midi_val: int) -> float:
    t = midi_val / (NUM_STEPS - 1)
    return ENV_TIME_MIN_MS + t * (ENV_TIME_MAX_MS - ENV_TIME_MIN_MS)

# Time to sample blocks
def ms_to_blocks(ms: float) -> float:
    blocks = (ms / 1000.0) * SAMPLE_RATE / AUDIO_BLOCK_SIZE
    return max(blocks, 1.0)  # minimum single block

# Coefficient for asymptotic segment
def redmon_coeff(tco: float, blocks: float) -> float:    
    return math.exp(-math.log((1.0 + tco) / tco) / blocks)


# Normalised float to unsigned fixed point.
def to_q15_unsigned(x: float) -> int:    
    val = round(x * 32768.0)
    return max(0, min(val, Q15_ONE))


# Float to signed fixed point.
def to_q15_signed(x: float) -> int:    
    val = round(x * 32768.0)
    val = max(Q15_MIN_SIGNED, min(val, Q15_MAX_SIGNED))
    return val & 0xFFFF  

# MIDI note number to frequency in Hz (equal temp)
def midi_to_freq_hz(midi_note: int) -> float:
    return A4_FREQ_HZ * (2.0 ** ((midi_note - A4_MIDI_NOTE) / 12.0))

# Frequency in Hz to phase increment (Frequency Control Word).
#   FCW = freq_hz * (PHASE_ACC_MAX / SAMPLE_RATE)
# This compensates for the inaccuracy in Fs caused by PLL
# limitations.
def freq_hz_to_fcw(freq_hz: float) -> int:
    fcw = round(freq_hz * (PHASE_ACC_MAX / SAMPLE_RATE))
    return max(0, min(fcw, PHASE_ACC_MAX - 1))

def midi_to_lfo_rate_hz(cc: int) -> float:
    return (cc / 127.0) * LFO_RATE_MAX_HZ

def lfo_rate_hz_to_inc(rate_hz: float) -> int:
    inc = round(rate_hz * AUDIO_BLOCK_SIZE / SAMPLE_RATE * PHASE_ACC_16_MAX)
    return max(0, min(inc, 0xFFFF))

# Find the index from which the remainder of the table is all zeroes.
def find_zero_run_cutoff(values_signed: list[int]) -> int:    
    n = len(values_signed)
    cutoff = n
    for i in range(n - 1, -1, -1):
        if values_signed[i] == 0:
            cutoff = i
        else:
            break
    return cutoff

# Outputs 
def generate_env_tables():
    attack_coeff = []
    attack_overshoot = []
    decay_coeff = []
    decay_overshoot_base_full = []
    release_overshoot_full = []

    # Calculate tables
    for midi_val in range(NUM_STEPS):
        ms = midi_to_ms(midi_val)
        blocks = ms_to_blocks(ms)

        a_coeff = redmon_coeff(ATTACK_TCO, blocks)
        d_coeff = redmon_coeff(DECAY_TCO, blocks)
        r_coeff = redmon_coeff(RELEASE_TCO, blocks)

        attack_coeff.append(to_q15_unsigned(a_coeff))

        a_over = (1.0 + ATTACK_TCO) * (1.0 - a_coeff)
        attack_overshoot.append(to_q15_unsigned(a_over))

        decay_coeff.append(to_q15_unsigned(d_coeff))

        d_over_base = -DECAY_TCO * (1.0 - d_coeff)
        decay_overshoot_base_full.append(to_q15_signed(d_over_base))

        r_over = -RELEASE_TCO * (1.0 - r_coeff)
        release_overshoot_full.append(to_q15_signed(r_over))

    # Truncate TCO tables at all-zeroes tail.    
    def as_signed16(bits: int) -> int:
        return bits - 0x10000 if bits & 0x8000 else bits

    decay_signed_vals = [as_signed16(v) for v in decay_overshoot_base_full]
    release_signed_vals = [as_signed16(v) for v in release_overshoot_full]

    decay_cutoff = find_zero_run_cutoff(decay_signed_vals)
    release_cutoff = find_zero_run_cutoff(release_signed_vals)

    decay_overshoot_base_truncated = decay_overshoot_base_full[:decay_cutoff]
    release_overshoot_truncated = release_overshoot_full[:release_cutoff]

    return {
        "env_attack_coeff_lut": (attack_coeff, "Q1_15", None),
        "env_attack_overshoot_lut": (attack_overshoot, "SQ1_15", None),
        "env_decay_coeff_lut": (decay_coeff, "Q1_15", None),
        "env_decay_overshoot_base_lut": (decay_overshoot_base_truncated, "SQ1_15", decay_cutoff),
        "env_release_overshoot_lut": (release_overshoot_truncated, "SQ1_15", release_cutoff),
    }

# Generates the MIDI note -> FCW (phase increment) table.
def generate_freq_table():
    fcw_table = []
    for midi_note in range(NUM_STEPS):
        freq_hz = midi_to_freq_hz(midi_note)
        fcw_table.append(freq_hz_to_fcw(freq_hz))

    return {
        "midi_fcw_lut": (fcw_table, "Q24_0", None)
    }

# Generates a generic exponential curve table used for non-linear controls
def generate_exp_curve_table():   
    generic_table = []
    for i in range(128):
        t = i / 127.0
        generic_table.append(to_q15_unsigned(t**EXPONENT))
    return {
        "midi_exp_curve_lut": (generic_table,"Q1_15",None)
    }

# Generates the phase increments for the LFO
def generate_lfo_phase_inc_table():
    lfo_phase_inc_table = []
    for i in range(128):
        inc = lfo_rate_hz_to_inc(midi_to_freq_hz(i))
        lfo_phase_inc_table.append(inc)    
    return {
        "lfo_inc_lut": (lfo_phase_inc_table,"uint16_t",None)
    }

    


# Output in c-array format for direct pasting into source file.
def format_c_array(name: str, values: list[int], c_type: str, per_line: int = 8) -> str:    
    hex_digits = {"uint32_t": 8, "int16_t": 4, "Q1_15": 4}.get(c_type, 4)

    lines = [f"const {c_type} {name}[{len(values)}] = {{"]
    for i in range(0, len(values), per_line):
        chunk = values[i:i + per_line]
        row = ", ".join(f"0x{v:0{hex_digits}X}" for v in chunk)
        lines.append(f"    {row},")
    lines.append("};")
    return "\n".join(lines)


# Entry point
def main():
    env_tables = generate_env_tables()
    freq_table = generate_freq_table()
    generic_curve_table = generate_exp_curve_table()
    lfo_inc_table = generate_lfo_phase_inc_table()

    print("/* ==================================================================")
    print(" *                Auto-generated by gen_luts.py")
    print(" * ==================================================================")
    print(f" * SAMPLE_RATE={SAMPLE_RATE}, AUDIO_BLOCK_SIZE={AUDIO_BLOCK_SIZE}")
    print(f" * ENV_TIME_MIN_MS={ENV_TIME_MIN_MS}, ENV_TIME_MAX_MS={ENV_TIME_MAX_MS}")
    print(f" * ATTACK_TCO={ATTACK_TCO:.6f}, DECAY_TCO=RELEASE_TCO={DECAY_TCO:.6f}")    
    print(" * ================================================================ */")
    print()    
    print(" * Paste the lokup tables into luts.c")
    print(" * Paste the 2 #defines into luts.h")
    print()    
    print(f"#define ENV_DECAY_OVERSHOOT_CUTOFF  {env_tables['env_decay_overshoot_base_lut'][2]}")
    print(f"#define ENV_RELEASE_OVERSHOOT_CUTOFF {env_tables['env_release_overshoot_lut'][2]}")
    print()

    for name, (values, c_type, _cutoff) in env_tables.items():
        print(format_c_array(name, values, c_type))
        print()

    # a_coeff, _, _ = tables["env_attack_coeff_lut"]
    # a_over, _, _ = tables["env_attack_overshoot_lut"]
    # r_over, _, r_cutoff = tables["env_release_overshoot_lut"]
    # print("/* DEBUG")    
    # for midi_val in (0, 64, 127):
    #     ms = midi_to_ms(midi_val)
    #     r_over_str = f"0x{r_over[midi_val]:04X}" if midi_val < r_cutoff else "0x0000 (truncated, implicit)"
    #     print(f" *   MIDI {midi_val:3d} -> {ms:8.2f} ms  "
    #           f"attack_coeff=0x{a_coeff[midi_val]:04X}  "
    #           f"attack_overshoot=0x{a_over[midi_val]:04X}  "
    #           f"release_overshoot={r_over_str}")
    # print(" */")                

    for name, (values, c_type, _cutoff) in freq_table.items():
        print(format_c_array(name, values, c_type))
        print()
        
    # fcw, _, _ = freq_tables["midi_fcw_lut"]
    # print("/* DEBUG FREQ TABLE")
    # for midi_note in (21, 60, 69, 108):  # A0, C4, A4, C8
    #     freq = midi_to_freq_hz(midi_note)
    #     print(f" *   MIDI {midi_note:3d} -> {freq:9.3f} Hz -> FCW 0x{fcw[midi_note]:08X}")
    # print(" */")

    for name, (values, c_type, _cutoff) in generic_curve_table.items():
        print(format_c_array(name, values, c_type))
        print()


    for name, (values, c_type, _cutoff) in lfo_inc_table.items():
        print(format_c_array(name, values, c_type))
        print()


if __name__ == "__main__":
    main()