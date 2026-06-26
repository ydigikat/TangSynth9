/*
 * (c) Jason Wilden, 2026
 */

#include "unity.h"
#include "voice.h"
#include "types.h"
#include "params.h"
#include "env.h"

static struct voice v;
static param_value_t params[PARAM_COUNT];

#define NOTE_A4   69
#define NOTE_C4   60
#define NOTE_E4   64
#define VEL_MF    64
#define VEL_FF   100

void setUp(void)
{
  param_create_default_patch(params);
  voice_init(&v, params);
}

void tearDown(void)
{

}

/* -----------------------------------------------------------------------
 * Helper: drive voice_tick() for N blocks
 * ----------------------------------------------------------------------- */
static void tick_n(struct voice *voice, int n)
{
  for (int i = 0; i < n; i++)
    voice_tick(voice);
}


/* -----------------------------------------------------------------------
 * Init and reset
 * ----------------------------------------------------------------------- */

 void test_init_state_is_idle(void)
{
  TEST_ASSERT_EQUAL(VOICE_IDLE, v.state);
}

void test_init_note_is_zero(void)
{
  TEST_ASSERT_EQUAL_UINT8(0, v.note);
}

void test_init_vel_is_zero(void)
{
  TEST_ASSERT_EQUAL_UINT8(0, v.vel);
}

void test_init_fcw_is_zero(void)
{
  TEST_ASSERT_EQUAL_UINT32(0, v.fcw);
}

void test_init_age_is_zero(void)
{
  TEST_ASSERT_EQUAL_INT8(0, v.age);
}

void test_init_event_flags_clear(void)
{
  TEST_ASSERT_EQUAL_UINT8(0, v.event_flags);
}

void test_init_amp_env_off(void)
{
  TEST_ASSERT_EQUAL(ENV_OFF, v.amp_env.state);
}

void test_reset_returns_to_idle(void)
{
  /* Force non-idle state then reset */
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);   /* consume START event */
  voice_reset(&v);
  TEST_ASSERT_EQUAL(VOICE_IDLE, v.state);
}

void test_reset_clears_note(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_reset(&v);
  TEST_ASSERT_EQUAL_UINT8(0, v.note);
}


/* -----------------------------------------------------------------------
 * note_on from IDLE → START FSM pathway
 * ----------------------------------------------------------------------- */

void test_note_on_from_idle_sets_active(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  TEST_ASSERT_EQUAL(VOICE_ACTIVE, v.state);
}

void test_note_on_from_idle_stores_note(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  TEST_ASSERT_EQUAL_UINT8(NOTE_A4, v.note);
}

void test_note_on_from_idle_stores_velocity(void)
{
  voice_note_on(&v, NOTE_A4, VEL_FF);
  TEST_ASSERT_EQUAL_UINT8(VEL_FF, v.vel);
}

void test_note_on_from_idle_sets_fcw(void)
{
  /* FCW should be non-zero for any real MIDI note */
  voice_note_on(&v, NOTE_A4, VEL_MF);
  TEST_ASSERT_NOT_EQUAL(0, v.fcw);
}

/*
 * After voice_tick() the START event should have been consumed and
 * the amp env should be in ATTACK (or DECAY for instant attack = 0).
 * The default patch has attack=0 so we land immediately in DECAY.
 */
void test_note_on_start_event_consumed_after_render(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);
  /* event_flags bit 0 (VOICE_EVENT_START) must be clear */
  TEST_ASSERT_EQUAL_UINT8(0, v.event_flags & 0x01);
}

void test_note_on_starts_amp_envelope(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);
  /* Default patch: attack=0, sustain=SQ15_ONE → should be in SUSTAIN */
  TEST_ASSERT_NOT_EQUAL(ENV_OFF, v.amp_env.state);
}

void test_note_on_different_notes_give_different_fcw(void)
{
  struct voice v2;
  voice_init(&v2, params);

  voice_note_on(&v,  NOTE_A4, VEL_MF);
  voice_note_on(&v2, NOTE_C4, VEL_MF);

  TEST_ASSERT_NOT_EQUAL(v.fcw, v2.fcw);
}

/* -----------------------------------------------------------------------
 * note_on from ACTIVE state
 * ----------------------------------------------------------------------- */

/* Same note → RETRIGGER */
void test_note_on_same_note_while_active_stays_active(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);
  voice_note_on(&v, NOTE_A4, VEL_FF);  /* same note */
  TEST_ASSERT_EQUAL(VOICE_ACTIVE, v.state);
}

void test_note_on_same_note_updates_velocity(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);
  voice_note_on(&v, NOTE_A4, VEL_FF);
  TEST_ASSERT_EQUAL_UINT8(VEL_FF, v.vel);
}

void test_note_on_same_note_sets_retrigger_flag(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);
  voice_note_on(&v, NOTE_A4, VEL_FF);
  /* VOICE_EVENT_RETRIGGER is bit 1 */
  TEST_ASSERT_NOT_EQUAL(0, v.event_flags & 0x02);
}

void test_retrigger_event_cleared_after_render(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);
  voice_note_on(&v, NOTE_A4, VEL_FF);
  voice_tick(&v);
  TEST_ASSERT_EQUAL_UINT8(0, v.event_flags & 0x02);
}

/* Different note → STEAL_RTZ */
void test_note_on_different_note_while_active_starts_steal(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);
  voice_note_on(&v, NOTE_C4, VEL_MF);
  TEST_ASSERT_EQUAL(VOICE_STEALING, v.state);
}

void test_steal_stores_steal_note(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);
  voice_note_on(&v, NOTE_C4, VEL_MF);
  TEST_ASSERT_EQUAL_UINT8(NOTE_C4, v.steal_note);
}

void test_steal_stores_steal_velocity(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);
  voice_note_on(&v, NOTE_C4, VEL_FF);
  TEST_ASSERT_EQUAL_UINT8(VEL_FF, v.steal_vel);
}

void test_steal_stores_steal_fcw(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);
  voice_note_on(&v, NOTE_C4, VEL_MF);
  TEST_ASSERT_NOT_EQUAL(0, v.steal_fcw);
}

void test_steal_fcw_differs_from_original(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);
  Q24_0 original_fcw = v.fcw;
  voice_note_on(&v, NOTE_C4, VEL_MF);
  TEST_ASSERT_NOT_EQUAL(original_fcw, v.steal_fcw);
}

/* -----------------------------------------------------------------------
 * note_on from STEALING (should be ignored / no-op)
 * ----------------------------------------------------------------------- */

void test_note_on_while_stealing_stays_stealing(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);
  voice_note_on(&v, NOTE_C4, VEL_MF);   /* enters STEALING */
  voice_note_on(&v, NOTE_E4, VEL_MF);   /* must be ignored */
  TEST_ASSERT_EQUAL(VOICE_STEALING, v.state);
}

void test_note_on_while_stealing_does_not_change_steal_note(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);
  voice_note_on(&v, NOTE_C4, VEL_MF);
  voice_note_on(&v, NOTE_E4, VEL_MF);
  /* steal_note should still be C4, not E4 */
  TEST_ASSERT_EQUAL_UINT8(NOTE_C4, v.steal_note);
}

/* -----------------------------------------------------------------------
 * note_off
 * ----------------------------------------------------------------------- */

void test_note_off_sets_release_flag(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);
  voice_note_off(&v);
  /* VOICE_EVENT_RELEASE is bit 2 */
  TEST_ASSERT_NOT_EQUAL(0, v.event_flags & 0x04);
}

void test_note_off_flag_consumed_after_render(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  voice_tick(&v);
  voice_note_off(&v);
  voice_tick(&v);
  TEST_ASSERT_EQUAL_UINT8(0, v.event_flags & 0x04);
}

/*
 * After note_off, the envelope moves to RELEASE state (level was > 0).
 * With default patch sustain = SQ15_ONE the level is non-zero.
 */
void test_note_off_puts_env_in_release(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  tick_n(&v, 3);          /* let envelope get some level */
  voice_note_off(&v);
  voice_tick(&v);         /* process the RELEASE event */
  TEST_ASSERT_EQUAL(ENV_RELEASE, v.amp_env.state);
}

/* -----------------------------------------------------------------------
 * render in IDLE — only UPDATE events are serviced
 * ----------------------------------------------------------------------- */

void test_render_idle_no_event_stays_idle(void)
{
  /* Voice starts idle, no events */
  voice_tick(&v);
  TEST_ASSERT_EQUAL(VOICE_IDLE, v.state);
}

void test_render_idle_with_no_event_does_not_crash(void)
{
  /* Smoke test: render idle voice many times without events */
  tick_n(&v, 32);
  TEST_ASSERT_EQUAL(VOICE_IDLE, v.state);
}

/* -----------------------------------------------------------------------
 * ACTIVE → IDLE on ENV_OFF
 *
 * Default patch: attack=0, decay=0, sustain=SQ15_ONE, release=0.
 * Sending note_off with release=0 means the envelope hits ENV_OFF on
 * the very next render call.  The voice should then return to IDLE.
 * ----------------------------------------------------------------------- */

void test_voice_returns_to_idle_after_env_off(void)
{
  /* Use instant release so ENV_OFF happens predictably */
  params[AMP_RELEASE] = 0;

  voice_note_on(&v, NOTE_A4, VEL_MF);
  tick_n(&v, 2);          /* settle into SUSTAIN */
  voice_note_off(&v);
  tick_n(&v, 2);          /* instant release → ENV_OFF → IDLE */

  TEST_ASSERT_EQUAL(VOICE_IDLE, v.state);
}

void test_event_flags_clear_when_idle_after_env_off(void)
{
  params[AMP_RELEASE] = 0;
  voice_note_on(&v, NOTE_A4, VEL_MF);
  tick_n(&v, 2);
  voice_note_off(&v);
  tick_n(&v, 2);
  TEST_ASSERT_EQUAL_UINT8(0, v.event_flags);
}

/* -----------------------------------------------------------------------
 * Steal sequence — RTZ then swap
 *
 * With instant attack/decay/release params the steal resolves in very
 * few render blocks and is deterministic.
 * ----------------------------------------------------------------------- */

void test_steal_completes_and_becomes_active(void)
{
  /* Fast envelope so shutdown resolves quickly */
  params[AMP_ATTACK]  = 0;
  params[AMP_DECAY]   = 0;
  params[AMP_RELEASE] = 0;

  voice_note_on(&v, NOTE_A4, VEL_MF);
  tick_n(&v, 2);

  voice_note_on(&v, NOTE_C4, VEL_MF);   /* trigger steal */
  TEST_ASSERT_EQUAL(VOICE_STEALING, v.state);

  /* Drive until steal completes (ACTIVE) or bail after 200 blocks */
  for (int i = 0; i < 200; i++)
  {
    if (v.state == VOICE_ACTIVE) break;
    voice_tick(&v);
  }

  TEST_ASSERT_EQUAL(VOICE_ACTIVE, v.state);
}

void test_steal_swaps_note_correctly(void)
{
  params[AMP_ATTACK]  = 0;
  params[AMP_DECAY]   = 0;
  params[AMP_RELEASE] = 0;

  voice_note_on(&v, NOTE_A4, VEL_MF);
  tick_n(&v, 2);
  voice_note_on(&v, NOTE_C4, VEL_MF);

  for (int i = 0; i < 200; i++)
  {
    if (v.state == VOICE_ACTIVE) break;
    voice_tick(&v);
  }

  TEST_ASSERT_EQUAL_UINT8(NOTE_C4, v.note);
}

void test_steal_swaps_velocity_correctly(void)
{
  params[AMP_ATTACK]  = 0;
  params[AMP_DECAY]   = 0;
  params[AMP_RELEASE] = 0;

  voice_note_on(&v, NOTE_A4, VEL_MF);
  tick_n(&v, 2);
  voice_note_on(&v, NOTE_C4, VEL_FF);

  for (int i = 0; i < 200; i++)
  {
    if (v.state == VOICE_ACTIVE) break;
    voice_tick(&v);
  }

  TEST_ASSERT_EQUAL_UINT8(VEL_FF, v.vel);
}

void test_steal_swaps_fcw_correctly(void)
{
  params[AMP_ATTACK]  = 0;
  params[AMP_DECAY]   = 0;
  params[AMP_RELEASE] = 0;

  voice_note_on(&v, NOTE_A4, VEL_MF);
  tick_n(&v, 2);

  /* Capture what FCW we expect for C4 */
  struct voice v_ref;
  voice_init(&v_ref, params);
  voice_note_on(&v_ref, NOTE_C4, VEL_MF);
  //Q24_0 expected_fcw = v_ref.steal_fcw; /* not set — grab from lut via note_on */

  voice_note_on(&v, NOTE_C4, VEL_MF);
  Q24_0 steal_fcw_set = v.steal_fcw;

  for (int i = 0; i < 200; i++)
  {
    if (v.state == VOICE_ACTIVE) break;
    voice_tick(&v);
  }

  TEST_ASSERT_EQUAL_UINT32(steal_fcw_set, v.fcw);
}

/* -----------------------------------------------------------------------
 * voice_update flag
 * ----------------------------------------------------------------------- */

void test_voice_update_sets_update_flag(void)
{
  voice_update(&v);
  /* VOICE_EVENT_UPDATE is bit 3 */
  TEST_ASSERT_NOT_EQUAL(0, v.event_flags & 0x08);
}

void test_voice_update_flag_consumed_in_idle_render(void)
{
  voice_update(&v);
  voice_tick(&v);   /* IDLE handler should consume UPDATE */
  TEST_ASSERT_EQUAL_UINT8(0, v.event_flags & 0x08);
}

void test_voice_update_flag_consumed_in_active_render(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  tick_n(&v, 2);
  voice_update(&v);
  voice_tick(&v);
  TEST_ASSERT_EQUAL_UINT8(0, v.event_flags & 0x08);
}

/* -----------------------------------------------------------------------
 * Modulator output 
 *
 * We can't easily predict exact Q1.15 values without reimplementing
 * the envelope maths here, but we can assert directional invariants.
 * ----------------------------------------------------------------------- */

/*
 * AMP_LEVEL mod output should be zero while voice is idle (ENV_OFF).
 */
void test_mod_value_amp_level_is_zero_while_idle(void)
{
  voice_tick(&v);
  TEST_ASSERT_EQUAL_INT16(0, v.mod_value[AMP_LEVEL]);
}

/*
 * After note_on + render, AMP_LEVEL should be non-zero.
 * (Default patch: instant attack, full sustain → immediately > 0.)
 */
void test_mod_value_amp_level_nonzero_after_note_on(void)
{
  voice_note_on(&v, NOTE_A4, VEL_MF);
  tick_n(&v, 2);
  TEST_ASSERT_NOT_EQUAL(0, v.mod_value[AMP_LEVEL]);
}

/*
 * After note_off with instant release, AMP_LEVEL should eventually
 * return to zero.
 */
void test_mod_value_amp_level_returns_to_zero_after_release(void)
{
  params[AMP_RELEASE] = 0;
  voice_note_on(&v, NOTE_A4, VEL_MF);
  tick_n(&v, 2);
  voice_note_off(&v);
  tick_n(&v, 5);
  TEST_ASSERT_EQUAL_INT16(0, v.mod_value[AMP_LEVEL]);
}



/* -----------------------------------------------------------------------
 * Test runner
 * ----------------------------------------------------------------------- */

int main(void)
{
  UNITY_BEGIN();

  /* Init / reset */
  RUN_TEST(test_init_state_is_idle);
  RUN_TEST(test_init_note_is_zero);
  RUN_TEST(test_init_vel_is_zero);
  RUN_TEST(test_init_fcw_is_zero);
  RUN_TEST(test_init_age_is_zero);
  RUN_TEST(test_init_event_flags_clear);
  RUN_TEST(test_init_amp_env_off);
  RUN_TEST(test_reset_returns_to_idle);
  RUN_TEST(test_reset_clears_note);

  /* IDLE->START */
  RUN_TEST(test_note_on_from_idle_sets_active);
  RUN_TEST(test_note_on_from_idle_stores_note);
  RUN_TEST(test_note_on_from_idle_stores_velocity);
  RUN_TEST(test_note_on_from_idle_sets_fcw);
  RUN_TEST(test_note_on_start_event_consumed_after_render);
  RUN_TEST(test_note_on_starts_amp_envelope);
  RUN_TEST(test_note_on_different_notes_give_different_fcw);

  /* note_on from ACTIVE */
  RUN_TEST(test_note_on_same_note_while_active_stays_active);
  RUN_TEST(test_note_on_same_note_updates_velocity);
  RUN_TEST(test_note_on_same_note_sets_retrigger_flag);
  RUN_TEST(test_retrigger_event_cleared_after_render);
  RUN_TEST(test_note_on_different_note_while_active_starts_steal);
  RUN_TEST(test_steal_stores_steal_note);
  RUN_TEST(test_steal_stores_steal_velocity);
  RUN_TEST(test_steal_stores_steal_fcw);
  RUN_TEST(test_steal_fcw_differs_from_original);

   /* note_on from STEALING */
  RUN_TEST(test_note_on_while_stealing_stays_stealing);
  RUN_TEST(test_note_on_while_stealing_does_not_change_steal_note);

  /* note_off */
  RUN_TEST(test_note_off_sets_release_flag);
  RUN_TEST(test_note_off_flag_consumed_after_render);
  RUN_TEST(test_note_off_puts_env_in_release);

  /* render idle */
  RUN_TEST(test_render_idle_no_event_stays_idle);
  RUN_TEST(test_render_idle_with_no_event_does_not_crash);

  /* ACTIVE → IDLE */
  RUN_TEST(test_voice_returns_to_idle_after_env_off);
  RUN_TEST(test_event_flags_clear_when_idle_after_env_off);

  /* Steal sequence */
  RUN_TEST(test_steal_completes_and_becomes_active);
  RUN_TEST(test_steal_swaps_note_correctly);
  RUN_TEST(test_steal_swaps_velocity_correctly);
  RUN_TEST(test_steal_swaps_fcw_correctly);

  /* voice_update */
  RUN_TEST(test_voice_update_sets_update_flag);
  RUN_TEST(test_voice_update_flag_consumed_in_idle_render);
  RUN_TEST(test_voice_update_flag_consumed_in_active_render);

  /* Modulator output */
  RUN_TEST(test_mod_value_amp_level_is_zero_while_idle);
  RUN_TEST(test_mod_value_amp_level_nonzero_after_note_on);
  RUN_TEST(test_mod_value_amp_level_returns_to_zero_after_release);
}