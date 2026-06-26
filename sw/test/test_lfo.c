/*
 * (c) Jason Wilden, 2026
 */

#include "unity.h"
#include "lfo.h"
#include "luts.h"
#include "types.h"

/* -----------------------------------------------------------------------
 * Fixture
 * ----------------------------------------------------------------------- */

static struct lfo l;

void setUp(void)
{
  lfo_init(&l);
}

void tearDown(void) {}

/* -----------------------------------------------------------------------
 * Helpers
 * ----------------------------------------------------------------------- */

/*
 * Manually set the phase accumulator and call lfo_render(), returning
 * all five outputs.  Leaves l.phase advanced by l.inc after the call.
 */
static void render_at(uint16_t phase,
                      SQ1_15 *tri, SQ1_15 *saw, SQ1_15 *ramp,
                      SQ1_15 *square, SQ1_15 *sandh)
{
  l.phase = phase;
  lfo_render(&l, tri, saw, ramp, square, sandh);
}

/*
 * Render N times with no phase manipulation — just let it run.
 */
static void render_n(int n)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  for (int i = 0; i < n; i++)
    lfo_render(&l, &tri, &saw, &ramp, &sq, &sh);
}

/* -----------------------------------------------------------------------
 * Initialisation
 * ----------------------------------------------------------------------- */

void test_init_phase_is_zero(void)
{
  TEST_ASSERT_EQUAL_UINT16(0, l.phase);
}

void test_init_prev_phase_is_zero(void)
{
  TEST_ASSERT_EQUAL_UINT16(0, l.prev_phase);
}

void test_init_sh_value_is_zero(void)
{
  TEST_ASSERT_EQUAL_INT16(0, l.sh_value);
}

void test_reset_clears_phase(void)
{
  l.phase = 0x8000;
  lfo_reset(&l);
  TEST_ASSERT_EQUAL_UINT16(0, l.phase);
}

void test_reset_clears_prev_phase(void)
{
  l.prev_phase = 0x8000;
  lfo_reset(&l);
  TEST_ASSERT_EQUAL_UINT16(0, l.prev_phase);
}

/* -----------------------------------------------------------------------
 * lfo_update()
 * ----------------------------------------------------------------------- */

void test_update_stores_rate(void)
{
  lfo_update(&l, 42, LFO_MODE_FREE);
  TEST_ASSERT_EQUAL_UINT8(42, l.rate);
}

void test_update_stores_mode(void)
{
  lfo_update(&l, 0, LFO_MODE_NOTE);
  TEST_ASSERT_EQUAL_INT(LFO_MODE_NOTE, l.mode);
}

void test_update_loads_inc_from_lut(void)
{
  lfo_update(&l, 10, LFO_MODE_FREE);
  TEST_ASSERT_EQUAL_UINT16(lfo_inc_lut[10], l.inc);
}

void test_update_rate_zero_gives_nonzero_inc(void)
{
  /* Slowest rate: inc must still be positive — the LFO always oscillates */
  lfo_update(&l, 0, LFO_MODE_FREE);
  TEST_ASSERT_GREATER_THAN_UINT16(0, l.inc);
}

void test_update_rate_127_gives_max_inc(void)
{
  /* Fastest rate: should match the last LUT entry */
  lfo_update(&l, 127, LFO_MODE_FREE);
  TEST_ASSERT_EQUAL_UINT16(lfo_inc_lut[127], l.inc);
}

void test_update_different_rates_give_different_inc(void)
{
  lfo_update(&l, 0, LFO_MODE_FREE);
  uint16_t slow = l.inc;
  lfo_update(&l, 64, LFO_MODE_FREE);
  uint16_t fast = l.inc;
  TEST_ASSERT_GREATER_THAN_UINT16(slow, fast);
}

/* -----------------------------------------------------------------------
 * Phase accumulation
 * ----------------------------------------------------------------------- */

void test_render_advances_phase_by_inc(void)
{
  lfo_update(&l, 10, LFO_MODE_FREE);
  uint16_t expected_inc = l.inc;

  SQ1_15 tri, saw, ramp, sq, sh;
  lfo_render(&l, &tri, &saw, &ramp, &sq, &sh);

  TEST_ASSERT_EQUAL_UINT16(expected_inc, l.phase);
}

void test_render_updates_prev_phase(void)
{
  lfo_update(&l, 10, LFO_MODE_FREE);
  SQ1_15 tri, saw, ramp, sq, sh;
  lfo_render(&l, &tri, &saw, &ramp, &sq, &sh);
  /* prev_phase should now be 0 (value before the advance) */
  TEST_ASSERT_EQUAL_UINT16(0, l.prev_phase);
}

void test_phase_wraps_naturally(void)
{
  /*
   * Set phase close to the top and give it a large inc so it wraps.
   * The wrapped phase must be less than it started.
   */
  l.inc = 0x1000;
  l.phase = 0xFF00;
  render_n(1);
  /* 0xFF00 + 0x1000 = 0x10F00, uint16 truncation → 0x0F00 */
  TEST_ASSERT_EQUAL_UINT16((uint16_t)(0xFF00u + 0x1000u), l.phase);
}

void test_phase_with_zero_inc_stays_zero(void)
{
  l.inc = 0;
  render_n(8);
  TEST_ASSERT_EQUAL_UINT16(0, l.phase);
}

/* -----------------------------------------------------------------------
 * Triangle waveform
 *
 * Formula (from lfo.c):
 *   folded = (phase < 0x8000) ? phase : (0xFFFF - phase)
 *   output = (SQ1_15)(folded * 2 - 0x7FFF)
 *
 * Test points:
 *   phase=0x0000 → folded=0      → 0*2-0x7FFF = -32767
 *   phase=0x4000 → folded=0x4000 → 0x4000*2-0x7FFF = 1
 *   phase=0x7FFF → folded=0x7FFF → 0x7FFF*2-0x7FFF = +32767
 *   phase=0x8000 → folded=0x7FFF → same as above = +32767
 *   phase=0xC000 → folded=0x3FFF → 0x3FFF*2-0x7FFF = -1
 *   phase=0xFFFF → folded=0      → -32767
 * ----------------------------------------------------------------------- */

void test_tri_at_phase_zero_is_negative_peak(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0x0000, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(-32767, tri);
}

void test_tri_at_quarter_cycle_is_near_zero(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0x4000, &tri, &saw, &ramp, &sq, &sh);
  /* folded=0x4000, 0x4000*2 - 0x7FFF = 0x8000 - 0x7FFF = 1 */
  TEST_ASSERT_EQUAL_INT16(1, tri);
}

void test_tri_at_half_cycle_is_positive_peak(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0x7FFF, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(32767, tri);
}

void test_tri_just_past_half_cycle_is_also_positive_peak(void)
{
  /* 0x8000 folds the same as 0x7FFF */
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0x8000, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(32767, tri);
}

void test_tri_at_three_quarter_cycle_is_near_zero(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  /* folded = 0xFFFF - 0xC000 = 0x3FFF; 0x3FFF*2 - 0x7FFF = -1 */
  render_at(0xC000, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(-1, tri);
}

void test_tri_at_cycle_end_is_negative_peak(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0xFFFF, &tri, &saw, &ramp, &sq, &sh);
  /* folded = 0xFFFF - 0xFFFF = 0; 0*2 - 0x7FFF = -32767 */
  TEST_ASSERT_EQUAL_INT16(-32767, tri);
}

void test_tri_first_half_is_monotonically_rising(void)
{
  SQ1_15 prev = -32768, cur;
  SQ1_15 saw, ramp, sq, sh;

  /* Step through phase 0 to 0x7FFF in 32 steps */
  for (int step = 0; step < 32; step++)
  {
    uint16_t phase = (uint16_t)(step * 0x7FFF / 31);
    render_at(phase, &cur, &saw, &ramp, &sq, &sh);
    TEST_ASSERT_GREATER_OR_EQUAL_INT16(prev, cur);
    prev = cur;
  }
}

void test_tri_second_half_is_monotonically_falling(void)
{
  SQ1_15 prev = 32767, cur;
  SQ1_15 saw, ramp, sq, sh;

  for (int step = 0; step <= 31; step++)
  {
    uint16_t phase = (uint16_t)(0x8000u + step * 0x7FFF / 31);
    render_at(phase, &cur, &saw, &ramp, &sq, &sh);
    TEST_ASSERT_LESS_OR_EQUAL_INT16(prev, cur);
    prev = cur;
  }
}

/* -----------------------------------------------------------------------
 * Sawtooth waveform
 *
 * Formula: output = (SQ1_15)phase  (reinterpret uint16 as int16)
 *
 *   phase=0x0000 → 0
 *   phase=0x7FFF → +32767
 *   phase=0x8000 → -32768  (two's complement wrap)
 *   phase=0xFFFF → -1
 * ----------------------------------------------------------------------- */

void test_saw_at_phase_zero_is_zero(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0x0000, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(0, saw);
}

void test_saw_at_positive_peak(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0x7FFF, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(32767, saw);
}

void test_saw_at_0x8000_is_negative_peak(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0x8000, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(-32768, saw);
}

void test_saw_at_end_of_cycle_is_minus_one(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0xFFFF, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(-1, saw);
}

/* -----------------------------------------------------------------------
 * Ramp (falling sawtooth) waveform
 *
 * Formula: output = (SQ1_15)(~phase)  i.e. 0xFFFF - phase then reinterpret
 *
 *   phase=0x0000 → ~0x0000 = 0xFFFF → (SQ1_15)0xFFFF = -1
 *   phase=0x7FFF → ~0x7FFF = 0x8000 → -32768
 *   phase=0x8000 → ~0x8000 = 0x7FFF → +32767
 *   phase=0xFFFF → ~0xFFFF = 0x0000 → 0
 * ----------------------------------------------------------------------- */

void test_ramp_at_phase_zero_is_minus_one(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0x0000, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(-1, ramp);
}

void test_ramp_at_just_before_halfway_is_negative_peak(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0x7FFF, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(-32768, ramp);
}

void test_ramp_at_halfway_is_positive_peak(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0x8000, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(32767, ramp);
}

void test_ramp_at_cycle_end_is_zero(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0xFFFF, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(0, ramp);
}

void test_saw_and_ramp_sum_to_minus_one(void)
{
  /*
   * Saw = (SQ1_15)phase, ramp = (SQ1_15)(~phase).
   * As int16: phase + (~phase) = phase + (0xFFFF - phase) = 0xFFFF = -1.
   * This holds at every phase point — a clean structural invariant.
   */
  uint16_t test_phases[] = {0x0000, 0x1234, 0x4000, 0x7FFF, 0x8000, 0xC000, 0xFFFF};
  for (int i = 0; i < 7; i++)
  {
    SQ1_15 tri, saw, ramp, sq, sh;
    render_at(test_phases[i], &tri, &saw, &ramp, &sq, &sh);
    TEST_ASSERT_EQUAL_INT16(-1, (int16_t)((int32_t)saw + (int32_t)ramp));
  }
}

/* -----------------------------------------------------------------------
 * Square waveform
 *
 * Formula: phase < 0x8000 → +32767, else → -32767
 * ----------------------------------------------------------------------- */

void test_square_is_positive_in_first_half(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0x0000, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(32767, sq);
}

void test_square_is_positive_just_before_midpoint(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0x7FFF, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(32767, sq);
}

void test_square_is_negative_at_midpoint(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0x8000, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(-32767, sq);
}

void test_square_is_negative_in_second_half(void)
{
  SQ1_15 tri, saw, ramp, sq, sh;
  render_at(0xFFFF, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(-32767, sq);
}

void test_square_output_is_only_two_values(void)
{
  /*
   * Step through 64 evenly-spaced phase points — output must always be
   * exactly +32767 or -32767, never anything in between.
   */
  SQ1_15 tri, saw, ramp, sq, sh;
  for (int i = 0; i < 64; i++)
  {
    uint16_t phase = (uint16_t)(i * 1024);
    render_at(phase, &tri, &saw, &ramp, &sq, &sh);
    TEST_ASSERT_TRUE(sq == 32767 || sq == -32767);
  }
}

/* -----------------------------------------------------------------------
 * Sample & Hold
 *
 * S&H should:
 *   - Output 0 on the first render (sh_value initialised to 0, no wrap yet)
 *   - Hold the same value while phase is advancing without wrapping
 *   - Produce a new value when a wrap is detected (phase < prev_phase)
 *   - New value is deterministic for a given PRNG seed (but we test for
 *     "changed" rather than exact value since the seed is static/internal)
 * ----------------------------------------------------------------------- */

void test_sandh_initial_output_is_zero(void)
{
  /*
   * At phase=0, prev_phase=0 initially: 0 < 0 is false, so no wrap, so
   * we return the initialised sh_value of 0.
   */
  SQ1_15 tri, saw, ramp, sq, sh;
  lfo_render(&l, &tri, &saw, &ramp, &sq, &sh);
  TEST_ASSERT_EQUAL_INT16(0, sh);
}

void test_sandh_holds_value_without_wrap(void)
{
  /*
   * Advance phase upward without wrapping over several renders.
   * S&H output must stay constant (no new random sample taken).
   */
  lfo_update(&l, 0, LFO_MODE_FREE);   /* slowest rate, definitely no wrap */
  SQ1_15 tri, saw, ramp, sq, sh0, sh1, sh2;

  lfo_render(&l, &tri, &saw, &ramp, &sq, &sh0);
  lfo_render(&l, &tri, &saw, &ramp, &sq, &sh1);
  lfo_render(&l, &tri, &saw, &ramp, &sq, &sh2);

  TEST_ASSERT_EQUAL_INT16(sh0, sh1);
  TEST_ASSERT_EQUAL_INT16(sh1, sh2);
}

void test_sandh_changes_value_on_wrap(void)
{
  /*
   * Render 1: phase=0xFF00, prev_phase=0 → no wrap (0xFF00 > 0)
   *           after: prev_phase=0xFF00, phase=0xFF00+0x0200=0x0100 (wrapped)
   *
   * Render 2: phase=0x0100 < prev_phase=0xFF00 → wrap detected, new sh_value drawn
   */
  l.inc = 0x0200;
  l.phase = 0xFF00;
  l.prev_phase = 0;

  SQ1_15 tri, saw, ramp, sq, sh;

  lfo_render(&l, &tri, &saw, &ramp, &sq, &sh);   /* performs the wrap */
  lfo_render(&l, &tri, &saw, &ramp, &sq, &sh);   /* detects the wrap  */

  /* sh_value must now be non-zero (xorshift32 never produces 0) */
  TEST_ASSERT_NOT_EQUAL(0, l.sh_value);
}

void test_sandh_second_wrap_gives_different_value(void)
{
  /*
   * Run two complete wrap cycles and confirm the S&H value changed.
   * Each wrap calls xorshift32 once — consecutive outputs of xorshift32
   * are never equal, so this is guaranteed to pass.
   *
   * Use inc=0x8001 so the phase wraps every 2 renders.
   * Render sequence:
   *   render 1: phase=0x0000 > prev=0       → no wrap; phase→0x8001
   *   render 2: phase=0x8001 > prev=0x0000  → no wrap; phase→0x0002 (wrapped)
   *   render 3: phase=0x0002 < prev=0x8001  → WRAP 1,  new sh_value
   *   render 4: phase=0x0003 < prev ... no, phase=0x0002+0x8001=0x8003 > 0x0002 → no wrap
   *   render 5: phase=0x8003+0x8001=0x0004 (wrapped) → next render detects it
   *   render 6: phase=0x0004 < prev=0x8003  → WRAP 2,  second sh_value
   */
  l.inc = 0x8001;
  l.phase = 0;
  l.prev_phase = 0;

  SQ1_15 tri, saw, ramp, sq, sh;

  /* Advance to first wrap detection */
  lfo_render(&l, &tri, &saw, &ramp, &sq, &sh);  /* 1 */
  lfo_render(&l, &tri, &saw, &ramp, &sq, &sh);  /* 2 */
  lfo_render(&l, &tri, &saw, &ramp, &sq, &sh);  /* 3 — wrap 1 detected */
  int16_t first_wrap_val = l.sh_value;

  /* Advance to second wrap detection */
  lfo_render(&l, &tri, &saw, &ramp, &sq, &sh);  /* 4 */
  lfo_render(&l, &tri, &saw, &ramp, &sq, &sh);  /* 5 */
  lfo_render(&l, &tri, &saw, &ramp, &sq, &sh);  /* 6 — wrap 2 detected */
  int16_t second_wrap_val = l.sh_value;

  TEST_ASSERT_NOT_EQUAL(first_wrap_val, second_wrap_val);
}

/* -----------------------------------------------------------------------
 * lfo_note_on() — phase reset behaviour
 * ----------------------------------------------------------------------- */

void test_note_on_resets_phase_in_note_mode(void)
{
  lfo_update(&l, 10, LFO_MODE_NOTE);
  l.phase = 0x4000;   /* mid-cycle */

  lfo_note_on(&l);

  TEST_ASSERT_EQUAL_UINT16(0, l.phase);
}

void test_note_on_resets_prev_phase_in_note_mode(void)
{
  lfo_update(&l, 10, LFO_MODE_NOTE);
  l.phase = 0x4000;
  l.prev_phase = 0x3000;

  lfo_note_on(&l);

  TEST_ASSERT_EQUAL_UINT16(0, l.prev_phase);
}

void test_note_on_does_not_reset_phase_in_free_mode(void)
{
  lfo_update(&l, 10, LFO_MODE_FREE);
  l.phase = 0x4000;

  lfo_note_on(&l);

  TEST_ASSERT_EQUAL_UINT16(0x4000, l.phase);
}

void test_note_on_free_mode_leaves_prev_phase_unchanged(void)
{
  lfo_update(&l, 10, LFO_MODE_FREE);
  l.prev_phase = 0x3000;

  lfo_note_on(&l);

  TEST_ASSERT_EQUAL_UINT16(0x3000, l.prev_phase);
}

void test_note_on_note_mode_restarts_waveform_from_beginning(void)
{
  /*
   * After a note_on reset, the first triangle output should be the
   * negative peak (same as a freshly initialised LFO).
   */
  lfo_update(&l, 10, LFO_MODE_NOTE);
  render_n(10);                        /* advance away from zero */
  lfo_note_on(&l);

  SQ1_15 tri, saw, ramp, sq, sh;
  lfo_render(&l, &tri, &saw, &ramp, &sq, &sh);

  TEST_ASSERT_EQUAL_INT16(-32767, tri);
}

/* -----------------------------------------------------------------------
 * Render produces valid range for all waveforms
 * ----------------------------------------------------------------------- */

void test_all_outputs_within_sq115_range(void)
{
  lfo_update(&l, 64, LFO_MODE_FREE);
  SQ1_15 tri, saw, ramp, sq, sh;

  for (int i = 0; i < 256; i++)
  {
    lfo_render(&l, &tri, &saw, &ramp, &sq, &sh);
    TEST_ASSERT_GREATER_OR_EQUAL_INT16(-32768, tri);
    TEST_ASSERT_LESS_OR_EQUAL_INT16(32767,    tri);
    TEST_ASSERT_GREATER_OR_EQUAL_INT16(-32768, saw);
    TEST_ASSERT_LESS_OR_EQUAL_INT16(32767,    saw);
    TEST_ASSERT_GREATER_OR_EQUAL_INT16(-32768, ramp);
    TEST_ASSERT_LESS_OR_EQUAL_INT16(32767,    ramp);
    TEST_ASSERT_GREATER_OR_EQUAL_INT16(-32768, sq);
    TEST_ASSERT_LESS_OR_EQUAL_INT16(32767,    sq);
    TEST_ASSERT_GREATER_OR_EQUAL_INT16(-32768, sh);
    TEST_ASSERT_LESS_OR_EQUAL_INT16(32767,    sh);
  }
}

/* -----------------------------------------------------------------------
 * Test runner
 * ----------------------------------------------------------------------- */

int main(void)
{
  UNITY_BEGIN();

  /* Init / reset */
  RUN_TEST(test_init_phase_is_zero);
  RUN_TEST(test_init_prev_phase_is_zero);
  RUN_TEST(test_init_sh_value_is_zero);
  RUN_TEST(test_reset_clears_phase);
  RUN_TEST(test_reset_clears_prev_phase);

  /* lfo_update() */
  RUN_TEST(test_update_stores_rate);
  RUN_TEST(test_update_stores_mode);
  RUN_TEST(test_update_loads_inc_from_lut);
  RUN_TEST(test_update_rate_zero_gives_nonzero_inc);
  RUN_TEST(test_update_rate_127_gives_max_inc);
  RUN_TEST(test_update_different_rates_give_different_inc);

  /* Phase accumulation */
  RUN_TEST(test_render_advances_phase_by_inc);
  RUN_TEST(test_render_updates_prev_phase);
  RUN_TEST(test_phase_wraps_naturally);
  RUN_TEST(test_phase_with_zero_inc_stays_zero);

  /* Triangle */
  RUN_TEST(test_tri_at_phase_zero_is_negative_peak);
  RUN_TEST(test_tri_at_quarter_cycle_is_near_zero);
  RUN_TEST(test_tri_at_half_cycle_is_positive_peak);
  RUN_TEST(test_tri_just_past_half_cycle_is_also_positive_peak);
  RUN_TEST(test_tri_at_three_quarter_cycle_is_near_zero);
  RUN_TEST(test_tri_at_cycle_end_is_negative_peak);
  RUN_TEST(test_tri_first_half_is_monotonically_rising);
  RUN_TEST(test_tri_second_half_is_monotonically_falling);

  /* Sawtooth */
  RUN_TEST(test_saw_at_phase_zero_is_zero);
  RUN_TEST(test_saw_at_positive_peak);
  RUN_TEST(test_saw_at_0x8000_is_negative_peak);
  RUN_TEST(test_saw_at_end_of_cycle_is_minus_one);

  /* Ramp */
  RUN_TEST(test_ramp_at_phase_zero_is_minus_one);
  RUN_TEST(test_ramp_at_just_before_halfway_is_negative_peak);
  RUN_TEST(test_ramp_at_halfway_is_positive_peak);
  RUN_TEST(test_ramp_at_cycle_end_is_zero);

  /* Saw + ramp structural invariant */
  RUN_TEST(test_saw_and_ramp_sum_to_minus_one);

  /* Square */
  RUN_TEST(test_square_is_positive_in_first_half);
  RUN_TEST(test_square_is_positive_just_before_midpoint);
  RUN_TEST(test_square_is_negative_at_midpoint);
  RUN_TEST(test_square_is_negative_in_second_half);
  RUN_TEST(test_square_output_is_only_two_values);

  /* S&H */
  RUN_TEST(test_sandh_initial_output_is_zero);
  RUN_TEST(test_sandh_holds_value_without_wrap);
  RUN_TEST(test_sandh_changes_value_on_wrap);
  RUN_TEST(test_sandh_second_wrap_gives_different_value);

  /* lfo_note_on() */
  RUN_TEST(test_note_on_resets_phase_in_note_mode);
  RUN_TEST(test_note_on_resets_prev_phase_in_note_mode);
  RUN_TEST(test_note_on_does_not_reset_phase_in_free_mode);
  RUN_TEST(test_note_on_free_mode_leaves_prev_phase_unchanged);
  RUN_TEST(test_note_on_note_mode_restarts_waveform_from_beginning);

  /* Range */
  RUN_TEST(test_all_outputs_within_sq115_range);

  return UNITY_END();
}