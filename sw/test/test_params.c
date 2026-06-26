/*
 * (c) Jason Wilden, 2026
 *
 * Unit tests for params.c
 *
 * Groups:
 *   1. param_init()              — CC map initialisation
 *   2. param_create_default_patch() — default patch values
 *   3. param_set_value_from_cc() — routing and conversion for each case class
 *   4. Inline helpers            — param_enum_from_cc, param_cc7bit_to_uni,
 *                                  param_cc7bit_to_bipolar (tested indirectly
 *                                  and directly)
 */

#include "unity.h"
#include "params.h"
#include "midi.h"
#include "luts.h"
#include "types.h"

static param_value_t params[PARAM_COUNT];

/* -------------------------------------------------------------------------
 * Fixture
 * ---------------------------------------------------------------------- */

void setUp(void)
{
  param_init();
  param_create_default_patch(params);
}

void tearDown(void) {}

/* =========================================================================
 * 1. param_init() — CC map sanity
 * ======================================================================= */

/*
 * A CC with no defined mapping must return PARAM_UNMAPPED without
 * touching the params array.  CC 0 (Bank Select) is not mapped.
 */
void test_init_unmapped_cc_returns_unmapped(void)
{
  param_value_t sentinel = params[AMP_LEVEL];
  enum param_mapping_type result = param_set_value_from_cc(0, 100, params);
  TEST_ASSERT_EQUAL(PARAM_UNMAPPED, result);
  TEST_ASSERT_EQUAL_UINT16(sentinel, params[AMP_LEVEL]);
}

/*
 * Every CC in the defined set must NOT return PARAM_UNMAPPED after init.
 * Spot-check a handful across the range.
 */
void test_init_volume_cc_is_mapped(void)
{
  enum param_mapping_type result = param_set_value_from_cc(MIDI_CC_VOLUME, 100, params);
  TEST_ASSERT_NOT_EQUAL(PARAM_UNMAPPED, result);
}

void test_init_attack_cc_is_mapped(void)
{
  enum param_mapping_type result = param_set_value_from_cc(MIDI_CC_ATTACKTIME, 64, params);
  TEST_ASSERT_NOT_EQUAL(PARAM_UNMAPPED, result);
}

void test_init_env1_attack_cc_is_mapped(void)
{
  /* CC 52 → ENV1_ATTACK */
  enum param_mapping_type result = param_set_value_from_cc(52, 64, params);
  TEST_ASSERT_NOT_EQUAL(PARAM_UNMAPPED, result);
}

void test_init_lfo1_rate_cc_is_mapped(void)
{
  /* CC 56 → LFO1_RATE */
  enum param_mapping_type result = param_set_value_from_cc(56, 64, params);
  TEST_ASSERT_NOT_EQUAL(PARAM_UNMAPPED, result);
}

/* =========================================================================
 * 2. param_create_default_patch()
 * ======================================================================= */

/* --- Amplifier ---------------------------------------------------------- */

void test_default_amp_level_is_sq15_one(void)
{
  TEST_ASSERT_EQUAL_UINT16(SQ15_ONE, params[AMP_LEVEL]);
}

void test_default_amp_sustain_is_sq15_one(void)
{
  TEST_ASSERT_EQUAL_UINT16(SQ15_ONE, params[AMP_SUSTAIN]);
}

void test_default_amp_attack_is_zero(void)
{
  TEST_ASSERT_EQUAL_UINT16(0, params[AMP_ATTACK]);
}

void test_default_amp_decay_is_zero(void)
{
  TEST_ASSERT_EQUAL_UINT16(0, params[AMP_DECAY]);
}

void test_default_amp_release_is_zero(void)
{
  TEST_ASSERT_EQUAL_UINT16(0, params[AMP_RELEASE]);
}

void test_default_amp_note_track_is_false(void)
{
  TEST_ASSERT_EQUAL_UINT16(false, params[AMP_NOTE_TRACK]);
}

void test_default_amp_vel_track_is_false(void)
{
  TEST_ASSERT_EQUAL_UINT16(false, params[AMP_VEL_TRACK]);
}

/* --- OSC1 --------------------------------------------------------------- */

void test_default_osc1_wave_is_saw(void)
{
  TEST_ASSERT_EQUAL_UINT16(OSC_SAW, params[OSC1_WAVE]);
}

void test_default_osc1_level_is_half(void)
{
  TEST_ASSERT_EQUAL_UINT16(SQ15_HALF, params[OSC1_LEVEL]);
}

void test_default_osc1_octave_is_zero(void)
{
  TEST_ASSERT_EQUAL_UINT16(0, params[OSC1_OCTAVE]);
}

void test_default_osc1_semi_is_zero(void)
{
  TEST_ASSERT_EQUAL_UINT16(0, params[OSC1_SEMI]);
}

void test_default_osc1_cents_is_zero(void)
{
  TEST_ASSERT_EQUAL_UINT16(0, params[OSC1_CENTS]);
}

/* --- OSC2 — the detuned oscillator ------------------------------------- */

void test_default_osc2_wave_is_saw(void)
{
  TEST_ASSERT_EQUAL_UINT16(OSC_SAW, params[OSC2_WAVE]);
}

void test_default_osc2_octave_is_one(void)
{
  TEST_ASSERT_EQUAL_UINT16(1, params[OSC2_OCTAVE]);
}

void test_default_osc2_cents_is_ten(void)
{
  TEST_ASSERT_EQUAL_UINT16(10, params[OSC2_CENTS]);
}

/* --- ENV1 --------------------------------------------------------------- */

void test_default_env1_mode_is_normal(void)
{
  TEST_ASSERT_EQUAL_UINT16(ENV_NORMAL, params[ENV1_MODE]);
}

void test_default_env1_sustain_is_sq15_one(void)
{
  TEST_ASSERT_EQUAL_UINT16(SQ15_ONE, params[ENV1_SUSTAIN]);
}

/* --- Filter ------------------------------------------------------------- */

void test_default_filt_cutoff_is_open(void)
{
  /* Default patch is explicitly wide open at 127 */
  TEST_ASSERT_EQUAL_UINT16(127, params[FILT_CUTOFF]);
}

void test_default_filt_res_is_zero(void)
{
  TEST_ASSERT_EQUAL_UINT16(0, params[FILT_RES]);
}

/* --- LFO ---------------------------------------------------------------- */

void test_default_lfo1_mode_is_note(void)
{
  TEST_ASSERT_EQUAL_UINT16(LFO_MODE_NOTE, params[LFO1_MODE]);
}

/* --- Global ------------------------------------------------------------- */

void test_default_voice_mode_is_poly(void)
{
  TEST_ASSERT_EQUAL_UINT16(VOICE_MODE_POLY, params[SYNTH_VOICE_MODE]);
}

void test_default_portamento_is_off(void)
{
  TEST_ASSERT_EQUAL_UINT16(false, params[SYNTH_PORTAMENTO_ON]);
}

/* =========================================================================
 * 3. param_set_value_from_cc() — routing and return values
 * ======================================================================= */

/* --- Return classification --------------------------------------------- */

/*
 * Voice params (index < VOICE_PARAM_COUNT) must return PARAM_VOICE.
 * AMP_ATTACK is a voice param.
 */
void test_cc_voice_param_returns_param_voice(void)
{
  enum param_mapping_type result = param_set_value_from_cc(MIDI_CC_ATTACKTIME, 64, params);
  TEST_ASSERT_EQUAL(PARAM_VOICE, result);
}

/*
 * Global params (index >= VOICE_PARAM_COUNT) must return PARAM_GLOBAL.
 * SYNTH_VOICE_MODE is mapped to MIDI_CC_POLYMODEON (127).
 */
void test_cc_global_param_returns_param_global(void)
{
  enum param_mapping_type result = param_set_value_from_cc(MIDI_CC_POLYMODEON, 0, params);
  TEST_ASSERT_EQUAL(PARAM_GLOBAL, result);
}

/* --- On/Off switches ---------------------------------------------------- */

/*
 * value > 64 → true.  MIDI convention: >= 64 is "on".  The code uses
 * strictly > 64, so 65 is the minimum "on" value.
 */
void test_switch_above_64_sets_true(void)
{
  param_set_value_from_cc(MIDI_CC_PORTAMENTO, 65, params);
  TEST_ASSERT_EQUAL_UINT16(true, params[SYNTH_PORTAMENTO_ON]);
}

void test_switch_at_64_sets_false(void)
{
  param_set_value_from_cc(MIDI_CC_PORTAMENTO, 64, params);
  TEST_ASSERT_EQUAL_UINT16(false, params[SYNTH_PORTAMENTO_ON]);
}

void test_switch_at_zero_sets_false(void)
{
  param_set_value_from_cc(MIDI_CC_PORTAMENTO, 0, params);
  TEST_ASSERT_EQUAL_UINT16(false, params[SYNTH_PORTAMENTO_ON]);
}

void test_switch_at_127_sets_true(void)
{
  param_set_value_from_cc(MIDI_CC_PORTAMENTO, 127, params);
  TEST_ASSERT_EQUAL_UINT16(true, params[SYNTH_PORTAMENTO_ON]);
}

/* --- Selectors — wave --------------------------------------------------- */

/*
 * param_enum_from_cc maps 0–127 onto 0..count-1.
 * For OSC_WAVE_COUNT==3: bucket width = 128/3 ≈ 42.
 *   value=0   → 0 (OSC_TRIANGLE)
 *   value=42  → 0 or 1 depending on exact integer arithmetic
 *   value=127 → clamped to OSC_WAVE_COUNT-1 = 2 (OSC_PULSE)
 * We test the unambiguous endpoints and the last bucket.
 */
void test_wave_selector_at_zero_gives_triangle(void)
{
  param_set_value_from_cc(15, 0, params);           /* CC 15 → OSC1_WAVE */
  TEST_ASSERT_EQUAL_UINT16(OSC_TRIANGLE, params[OSC1_WAVE]);
}

void test_wave_selector_at_127_gives_pulse(void)
{
  param_set_value_from_cc(15, 127, params);
  TEST_ASSERT_EQUAL_UINT16(OSC_PULSE, params[OSC1_WAVE]);
}

void test_wave_selector_result_in_range(void)
{
  param_set_value_from_cc(15, 64, params);
  TEST_ASSERT_LESS_THAN_UINT16(OSC_WAVE_COUNT, params[OSC1_WAVE]);
}

void test_osc2_wave_selector_mirrors_osc1(void)
{
  /* Both CCs use the same conversion; they must produce identical results
   * for the same incoming value. */
  param_set_value_from_cc(15, 90, params);   /* OSC1_WAVE */
  param_set_value_from_cc(23, 90, params);   /* OSC2_WAVE */
  TEST_ASSERT_EQUAL_UINT16(params[OSC1_WAVE], params[OSC2_WAVE]);
}

/* --- Selectors — env mode ---------------------------------------------- */

void test_env_mode_selector_at_zero_gives_normal(void)
{
  param_set_value_from_cc(31, 0, params);           /* CC 31 → ENV1_MODE */
  TEST_ASSERT_EQUAL_UINT16(ENV_NORMAL, params[ENV1_MODE]);
}

void test_env_mode_selector_at_127_gives_last_mode(void)
{
  param_set_value_from_cc(31, 127, params);
  TEST_ASSERT_EQUAL_UINT16(ENV_MODE_COUNT - 1, params[ENV1_MODE]);
}

void test_env_mode_selector_result_in_range(void)
{
  param_set_value_from_cc(31, 64, params);
  TEST_ASSERT_LESS_THAN_UINT16(ENV_MODE_COUNT, params[ENV1_MODE]);
}

/* --- Selectors — LFO mode ---------------------------------------------- */

void test_lfo_mode_selector_at_zero_gives_free(void)
{
  param_set_value_from_cc(57, 0, params);
  TEST_ASSERT_EQUAL_UINT16(LFO_MODE_FREE, params[LFO1_MODE]);
}

void test_lfo_mode_selector_result_in_range(void)
{
  param_set_value_from_cc(57, 127, params);
  TEST_ASSERT_LESS_THAN_UINT16(LFO_MODE_COUNT, params[LFO1_MODE]);
}

/* --- Unipolar linear ---------------------------------------------------- */

/*
 * param_cc7bit_to_uni: maps 0→0, 127→Q15_ONE.
 * AMP_SUSTAIN is mapped to CC 14.
 */
void test_uni_at_zero_is_zero(void)
{
  param_set_value_from_cc(14, 0, params);           /* CC 14 → AMP_SUSTAIN */
  TEST_ASSERT_EQUAL_UINT16(0, params[AMP_SUSTAIN]);
}

void test_uni_at_127_is_q15_one(void)
{
  param_set_value_from_cc(14, 127, params);
  TEST_ASSERT_EQUAL_UINT16(Q15_ONE, params[AMP_SUSTAIN]);
}

void test_uni_at_64_is_roughly_half(void)
{
  /* 64 * Q15_ONE / 127 = 64 * 32767 / 127 = 16512 — not SQ15_HALF.
   * True mid-scale on a 7-bit CC is CC 63.5, so CC 64 lands slightly
   * above Q15 half.  Check the exact integer result. */
  param_set_value_from_cc(14, 64, params);
  TEST_ASSERT_EQUAL_UINT16(16512, params[AMP_SUSTAIN]);
}

/* PW uses the same path */
void test_osc_pw_at_127_is_q15_one(void)
{
  param_set_value_from_cc(20, 127, params);          /* CC 20 → OSC1_PW */
  TEST_ASSERT_EQUAL_UINT16(Q15_ONE, params[OSC1_PW]);
}

/* --- Bipolar — octave (range ±2) --------------------------------------- */

/*
 * param_cc7bit_to_bipolar with range=2:
 *   cc=0   → (0-64)*2/64   = -2
 *   cc=64  → (64-64)*2/64  =  0
 *   cc=127 → (127-64)*2/64 = ~1 (63*2/64 = 1 via integer truncation)
 *
 * The result is cast to int8_t then stored as uint16_t — reinterpret
 * the stored value as int8_t to check the signed result.
 */
void test_octave_at_cc64_is_zero(void)
{
  param_set_value_from_cc(17, 64, params);           /* CC 17 → OSC1_OCTAVE */
  TEST_ASSERT_EQUAL_INT8(0, (int8_t)params[OSC1_OCTAVE]);
}

void test_octave_at_cc0_is_negative(void)
{
  param_set_value_from_cc(17, 0, params);
  TEST_ASSERT_LESS_THAN_INT8(0, (int8_t)params[OSC1_OCTAVE]);
}

void test_octave_at_cc0_is_minus_two(void)
{
  param_set_value_from_cc(17, 0, params);
  TEST_ASSERT_EQUAL_INT8(-2, (int8_t)params[OSC1_OCTAVE]);
}

void test_octave_at_cc127_is_positive(void)
{
  param_set_value_from_cc(17, 127, params);
  TEST_ASSERT_GREATER_THAN_INT8(0, (int8_t)params[OSC1_OCTAVE]);
}

/* --- Bipolar — semi (range ±6) ----------------------------------------- */

void test_semi_at_cc64_is_zero(void)
{
  param_set_value_from_cc(18, 64, params);           /* CC 18 → OSC1_SEMI */
  TEST_ASSERT_EQUAL_INT8(0, (int8_t)params[OSC1_SEMI]);
}

void test_semi_at_cc0_is_minus_six(void)
{
  param_set_value_from_cc(18, 0, params);
  TEST_ASSERT_EQUAL_INT8(-6, (int8_t)params[OSC1_SEMI]);
}

void test_semi_at_cc127_is_positive(void)
{
  param_set_value_from_cc(18, 127, params);
  TEST_ASSERT_GREATER_THAN_INT8(0, (int8_t)params[OSC1_SEMI]);
}

/* --- Bipolar — cents (range ±50) --------------------------------------- */

void test_cents_at_cc64_is_zero(void)
{
  param_set_value_from_cc(19, 64, params);           /* CC 19 → OSC1_CENTS */
  TEST_ASSERT_EQUAL_INT8(0, (int8_t)params[OSC1_CENTS]);
}

void test_cents_at_cc0_is_minus_50(void)
{
  param_set_value_from_cc(19, 0, params);
  TEST_ASSERT_EQUAL_INT8(-50, (int8_t)params[OSC1_CENTS]);
}

void test_cents_at_cc127_is_positive_and_in_range(void)
{
  param_set_value_from_cc(19, 127, params);
  int8_t v = (int8_t)params[OSC1_CENTS];
  TEST_ASSERT_GREATER_THAN_INT8(0, v);
  TEST_ASSERT_LESS_OR_EQUAL_INT8(50, v);
}

/* OSC2 cents must go through the same conversion */
void test_osc2_cents_at_cc64_is_zero(void)
{
  param_set_value_from_cc(27, 64, params);           /* CC 27 → OSC2_CENTS */
  TEST_ASSERT_EQUAL_INT8(0, (int8_t)params[OSC2_CENTS]);
}

/* --- Exponential (LUT) controls ---------------------------------------- */

/*
 * AMP_LEVEL maps to midi_exp_curve_lut[value].
 * We check that the stored value equals the LUT entry directly.
 */
void test_exp_lut_at_zero_gives_lut_entry(void)
{
  param_set_value_from_cc(MIDI_CC_VOLUME, 0, params);
  TEST_ASSERT_EQUAL_UINT16(midi_exp_curve_lut[0], params[AMP_LEVEL]);
}

void test_exp_lut_at_127_gives_lut_entry(void)
{
  param_set_value_from_cc(MIDI_CC_VOLUME, 127, params);
  TEST_ASSERT_EQUAL_UINT16(midi_exp_curve_lut[127], params[AMP_LEVEL]);
}

void test_exp_lut_is_monotonic_sample(void)
{
  /* Spot-check monotonicity at a few points */
  param_set_value_from_cc(MIDI_CC_VOLUME, 32, params);
  uint16_t low = params[AMP_LEVEL];
  param_set_value_from_cc(MIDI_CC_VOLUME, 96, params);
  uint16_t high = params[AMP_LEVEL];
  TEST_ASSERT_GREATER_THAN_UINT16(low, high);
}

/* LFO1_RATE also goes through the exp LUT (CC 56) */
void test_lfo_rate_at_127_is_max_lut_entry(void)
{
  param_set_value_from_cc(56, 127, params);
  TEST_ASSERT_EQUAL_UINT16(midi_exp_curve_lut[127], params[LFO1_RATE]);
}

/* --- Raw passthrough (AMP_ATTACK / FILT_CUTOFF etc.) ------------------- */

/*
 * These store the raw 7-bit CC value unchanged.
 */
void test_raw_attack_stores_value_unchanged(void)
{
  param_set_value_from_cc(MIDI_CC_ATTACKTIME, 42, params);
  TEST_ASSERT_EQUAL_UINT16(42, params[AMP_ATTACK]);
}

void test_raw_decay_stores_value_unchanged(void)
{
  param_set_value_from_cc(MIDI_CC_DECAYTIME, 100, params);
  TEST_ASSERT_EQUAL_UINT16(100, params[AMP_DECAY]);
}

void test_raw_release_stores_value_unchanged(void)
{
  param_set_value_from_cc(MIDI_CC_RELEASETIME, 77, params);
  TEST_ASSERT_EQUAL_UINT16(77, params[AMP_RELEASE]);
}

void test_raw_filt_cutoff_stores_value_unchanged(void)
{
  /* FILT_CUTOFF is not mapped in param_map (no CC assignment in param_init).
   * If/when it is mapped this test will need updating.  For now verify the
   * default value is preserved when an unmapped CC is received. */
  param_set_value_from_cc(0, 64, params);            /* CC 0 = unmapped */
  TEST_ASSERT_EQUAL_UINT16(127, params[FILT_CUTOFF]); /* default unchanged */
}

/* --- param_set_value_from_cc does not disturb other params ------------- */

/*
 * Writing one param must not corrupt a neighbour in the array.
 */
void test_setting_osc1_wave_does_not_disturb_osc1_level(void)
{
  param_value_t level_before = params[OSC1_LEVEL];
  param_set_value_from_cc(15, 64, params);            /* CC 15 → OSC1_WAVE */
  TEST_ASSERT_EQUAL_UINT16(level_before, params[OSC1_LEVEL]);
}

void test_setting_amp_attack_does_not_disturb_amp_sustain(void)
{
  param_value_t sustain_before = params[AMP_SUSTAIN];
  param_set_value_from_cc(MIDI_CC_ATTACKTIME, 99, params);
  TEST_ASSERT_EQUAL_UINT16(sustain_before, params[AMP_SUSTAIN]);
}

/* =========================================================================
 * Test runner
 * ======================================================================= */

int main(void)
{
  UNITY_BEGIN();

  /* param_init */
  RUN_TEST(test_init_unmapped_cc_returns_unmapped);
  RUN_TEST(test_init_volume_cc_is_mapped);
  RUN_TEST(test_init_attack_cc_is_mapped);
  RUN_TEST(test_init_env1_attack_cc_is_mapped);
  RUN_TEST(test_init_lfo1_rate_cc_is_mapped);

  /* param_create_default_patch */
  RUN_TEST(test_default_amp_level_is_sq15_one);
  RUN_TEST(test_default_amp_sustain_is_sq15_one);
  RUN_TEST(test_default_amp_attack_is_zero);
  RUN_TEST(test_default_amp_decay_is_zero);
  RUN_TEST(test_default_amp_release_is_zero);
  RUN_TEST(test_default_amp_note_track_is_false);
  RUN_TEST(test_default_amp_vel_track_is_false);
  RUN_TEST(test_default_osc1_wave_is_saw);
  RUN_TEST(test_default_osc1_level_is_half);
  RUN_TEST(test_default_osc1_octave_is_zero);
  RUN_TEST(test_default_osc1_semi_is_zero);
  RUN_TEST(test_default_osc1_cents_is_zero);
  RUN_TEST(test_default_osc2_wave_is_saw);
  RUN_TEST(test_default_osc2_octave_is_one);
  RUN_TEST(test_default_osc2_cents_is_ten);
  RUN_TEST(test_default_env1_mode_is_normal);
  RUN_TEST(test_default_env1_sustain_is_sq15_one);
  RUN_TEST(test_default_filt_cutoff_is_open);
  RUN_TEST(test_default_filt_res_is_zero);
  RUN_TEST(test_default_lfo1_mode_is_note);
  RUN_TEST(test_default_voice_mode_is_poly);
  RUN_TEST(test_default_portamento_is_off);

  /* return value routing */
  RUN_TEST(test_cc_voice_param_returns_param_voice);
  RUN_TEST(test_cc_global_param_returns_param_global);

  /* switches */
  RUN_TEST(test_switch_above_64_sets_true);
  RUN_TEST(test_switch_at_64_sets_false);
  RUN_TEST(test_switch_at_zero_sets_false);
  RUN_TEST(test_switch_at_127_sets_true);

  /* wave selector */
  RUN_TEST(test_wave_selector_at_zero_gives_triangle);
  RUN_TEST(test_wave_selector_at_127_gives_pulse);
  RUN_TEST(test_wave_selector_result_in_range);
  RUN_TEST(test_osc2_wave_selector_mirrors_osc1);

  /* env mode selector */
  RUN_TEST(test_env_mode_selector_at_zero_gives_normal);
  RUN_TEST(test_env_mode_selector_at_127_gives_last_mode);
  RUN_TEST(test_env_mode_selector_result_in_range);

  /* LFO mode selector */
  RUN_TEST(test_lfo_mode_selector_at_zero_gives_free);
  RUN_TEST(test_lfo_mode_selector_result_in_range);

  /* unipolar linear */
  RUN_TEST(test_uni_at_zero_is_zero);
  RUN_TEST(test_uni_at_127_is_q15_one);
  RUN_TEST(test_uni_at_64_is_roughly_half);
  RUN_TEST(test_osc_pw_at_127_is_q15_one);

  /* bipolar octave */
  RUN_TEST(test_octave_at_cc64_is_zero);
  RUN_TEST(test_octave_at_cc0_is_negative);
  RUN_TEST(test_octave_at_cc0_is_minus_two);
  RUN_TEST(test_octave_at_cc127_is_positive);

  /* bipolar semi */
  RUN_TEST(test_semi_at_cc64_is_zero);
  RUN_TEST(test_semi_at_cc0_is_minus_six);
  RUN_TEST(test_semi_at_cc127_is_positive);

  /* bipolar cents */
  RUN_TEST(test_cents_at_cc64_is_zero);
  RUN_TEST(test_cents_at_cc0_is_minus_50);
  RUN_TEST(test_cents_at_cc127_is_positive_and_in_range);
  RUN_TEST(test_osc2_cents_at_cc64_is_zero);

  /* exponential LUT */
  RUN_TEST(test_exp_lut_at_zero_gives_lut_entry);
  RUN_TEST(test_exp_lut_at_127_gives_lut_entry);
  RUN_TEST(test_exp_lut_is_monotonic_sample);
  RUN_TEST(test_lfo_rate_at_127_is_max_lut_entry);

  /* raw passthrough */
  RUN_TEST(test_raw_attack_stores_value_unchanged);
  RUN_TEST(test_raw_decay_stores_value_unchanged);
  RUN_TEST(test_raw_release_stores_value_unchanged);
  RUN_TEST(test_raw_filt_cutoff_stores_value_unchanged);

  /* isolation */
  RUN_TEST(test_setting_osc1_wave_does_not_disturb_osc1_level);
  RUN_TEST(test_setting_amp_attack_does_not_disturb_amp_sustain);

  return UNITY_END();
}