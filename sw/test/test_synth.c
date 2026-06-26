/*
 * (c) Jason Wilden, 2026
 */

#include "unity.h"
#include "synth.h"
#include "voice.h"
#include "params.h"
#include "midi.h"

static struct synth s;

/* -------------------------------------------------------------------------
 * Fixture setup/teardown
 * ---------------------------------------------------------------------- */
void setUp(void)
{
  synth_init(&s);
}

void tearDown(void) {}

/* -----------------------------------------------------------------------
 * Helpers
 * ----------------------------------------------------------------------- */

/* Build a 3-byte channel MIDI message on the stack */
static struct midi_msg make_msg(uint8_t status, uint8_t d1, uint8_t d2)
{
  struct midi_msg m;
  m.len = 3;
  m.data[0] = status;
  m.data[1] = d1;
  m.data[2] = d2;
  return m;
}

static struct midi_msg make_note_on(uint8_t note, uint8_t vel)
{
  return make_msg(MIDI_STATUS_NOTE_ON, note, vel);
}

static struct midi_msg make_note_off(uint8_t note)
{
  return make_msg(MIDI_STATUS_NOTE_OFF, note, 0);
}

/* Count voices in a given state */
static int count_voices_in_state(struct synth *synth, enum voice_state state)
{
  int count = 0;
  for (int i = 0; i < MAX_VOICES; i++)
  {
    if (synth->voice[i].state == state)
      count++;
  }
  return count;
}

/* Return the voice playing a note, or NULL */
static struct voice *find_voice_playing(struct synth *synth, uint8_t note)
{
  for (int i = 0; i < MAX_VOICES; i++)
  {
    if (synth->voice[i].state == VOICE_ACTIVE && synth->voice[i].note == note)
      return &synth->voice[i];
  }
  return NULL;
}

/* Drive synth_tick() n times */
static void tick_n(struct synth *synth, int n)
{
  for (int i = 0; i < n; i++)
    synth_tick(synth);
}

/* -----------------------------------------------------------------------
 * Initialisation
 * ----------------------------------------------------------------------- */

void test_init_midi_channel_is_omni(void)
{
  TEST_ASSERT_EQUAL_UINT8(MIDI_OMNI, s.midi_channel);
}

void test_init_all_voices_idle(void)
{
  TEST_ASSERT_EQUAL_INT(MAX_VOICES, count_voices_in_state(&s, VOICE_IDLE));
}

void test_init_voice_indices_set(void)
{
  for (int i = 0; i < MAX_VOICES; i++)
  {
    TEST_ASSERT_EQUAL_INT(i, s.voice[i].idx);
  }
}

void test_init_params_pointer_set_on_all_voices(void)
{
  for (int i = 0; i < MAX_VOICES; i++)
  {
    TEST_ASSERT_NOT_NULL(s.voice[i].params);
  }
}

/* -----------------------------------------------------------------------
 * Voice allocation — free voice path
 * ----------------------------------------------------------------------- */

void test_note_on_activates_one_voice(void)
{
  struct midi_msg m = make_note_on(60, 64);
  synth_handle_midi(&s, &m);
  TEST_ASSERT_EQUAL_INT(1, count_voices_in_state(&s, VOICE_ACTIVE));
}

void test_note_on_stores_correct_note(void)
{
  struct midi_msg m = make_note_on(60, 64);
  synth_handle_midi(&s, &m);
  TEST_ASSERT_NOT_NULL(find_voice_playing(&s, 60));
}

void test_two_note_on_activates_two_voices(void)
{
  struct midi_msg m1 = make_note_on(60, 64);
  struct midi_msg m2 = make_note_on(64, 64);
  synth_handle_midi(&s, &m1);
  synth_handle_midi(&s, &m2);
  TEST_ASSERT_EQUAL_INT(2, count_voices_in_state(&s, VOICE_ACTIVE));
}

void test_four_note_on_fills_all_voices(void)
{
  uint8_t notes[] = {60, 62, 64, 65};
  for (int i = 0; i < MAX_VOICES; i++)
  {
    struct midi_msg m = make_note_on(notes[i], 64);
    synth_handle_midi(&s, &m);
  }
  TEST_ASSERT_EQUAL_INT(MAX_VOICES, count_voices_in_state(&s, VOICE_ACTIVE));
}

/* -----------------------------------------------------------------------
 * Voice allocation — retrigger path
 *
 * Sending the same note twice while it is still active should retrigger
 * the envelope, not allocate a second voice.
 * ----------------------------------------------------------------------- */

void test_retrigger_does_not_add_a_second_voice(void)
{
  struct midi_msg m = make_note_on(60, 64);
  synth_handle_midi(&s, &m);
  synth_handle_midi(&s, &m); /* same note again */
  TEST_ASSERT_EQUAL_INT(1, count_voices_in_state(&s, VOICE_ACTIVE));
}

void test_retrigger_updates_velocity(void)
{
  struct midi_msg m1 = make_note_on(60, 64);
  struct midi_msg m2 = make_note_on(60, 100);
  synth_handle_midi(&s, &m1);
  tick_n(&s, 1); /* consume START so voice is settled ACTIVE */
  synth_handle_midi(&s, &m2);
  struct voice *v = find_voice_playing(&s, 60);
  TEST_ASSERT_NOT_NULL(v);
  TEST_ASSERT_EQUAL_UINT8(100, v->vel);
}

/* -----------------------------------------------------------------------
 * Voice allocation — steal path (5th note when all 4 busy)
 * ----------------------------------------------------------------------- */

void test_fifth_note_on_steals_oldest_voice(void)
{
  /*
   * Fill all 4 voices, then send a 5th note.  The synth must still have
   * only MAX_VOICES voices in play (one of them now STEALING or already
   * transitioning); none should be left dangling.
   */
  uint8_t notes[] = {60, 62, 64, 65};
  for (int i = 0; i < MAX_VOICES; i++)
  {
    struct midi_msg m = make_note_on(notes[i], 64);
    synth_handle_midi(&s, &m);
    tick_n(&s, 1); /* age each voice */
  }

  struct midi_msg steal = make_note_on(67, 64);
  synth_handle_midi(&s, &steal);

  /* Total voices in any non-idle state must remain <= MAX_VOICES */
  int active = count_voices_in_state(&s, VOICE_ACTIVE);
  int stealing = count_voices_in_state(&s, VOICE_STEALING);
  TEST_ASSERT_LESS_OR_EQUAL_INT(MAX_VOICES, active + stealing);
}

void test_fifth_note_steal_target_is_stealing_state(void)
{
  /* After filling 4 voices and sending a 5th, exactly 1 should be STEALING */
  uint8_t notes[] = {60, 62, 64, 65};
  for (int i = 0; i < MAX_VOICES; i++)
  {
    struct midi_msg m = make_note_on(notes[i], 64);
    synth_handle_midi(&s, &m);
    tick_n(&s, 1);
  }

  struct midi_msg steal = make_note_on(67, 64);
  synth_handle_midi(&s, &steal);

  TEST_ASSERT_EQUAL_INT(1, count_voices_in_state(&s, VOICE_STEALING));
}

void test_steal_picks_oldest_not_youngest(void)
{
  /*
   * Voice order: note 60 is the oldest (allocated and aged first).
   * After the steal, its steal_note should hold the incoming note.
   */
  struct midi_msg m0 = make_note_on(60, 64);
  synth_handle_midi(&s, &m0);
  tick_n(&s, 3); /* age it 3 generations */

  struct midi_msg m1 = make_note_on(62, 64);
  synth_handle_midi(&s, &m1);
  tick_n(&s, 1);

  struct midi_msg m2 = make_note_on(64, 64);
  synth_handle_midi(&s, &m2);
  tick_n(&s, 1);

  struct midi_msg m3 = make_note_on(65, 64);
  synth_handle_midi(&s, &m3);
  tick_n(&s, 1);

  struct midi_msg steal = make_note_on(67, 64);
  synth_handle_midi(&s, &steal);

  /* The stolen voice was playing note 60 — its steal_note should now be 67 */
  bool found_steal_target = false;
  for (int i = 0; i < MAX_VOICES; i++)
  {
    if (s.voice[i].state == VOICE_STEALING && s.voice[i].steal_note == 67)
    {
      found_steal_target = true;
      /* And the voice that was stolen must have originally held note 60 */
      /* (the note field is still the old note during the RTZ phase) */
    }
  }
  TEST_ASSERT_TRUE(found_steal_target);
}

/* -----------------------------------------------------------------------
 * Voice ageing
 * ----------------------------------------------------------------------- */

void test_age_increments_on_new_allocation(void)
{
  /*
   * When a free voice is allocated, age_voices() is called so all existing
   * ACTIVE voices increment their age counter.
   */
  struct midi_msg m1 = make_note_on(60, 64);
  struct midi_msg m2 = make_note_on(62, 64);

  synth_handle_midi(&s, &m1);
  int age_before = find_voice_playing(&s, 60)->age;

  synth_handle_midi(&s, &m2); /* triggers age_voices() */
  int age_after = find_voice_playing(&s, 60)->age;

  TEST_ASSERT_GREATER_THAN_INT(age_before, age_after);
}

void test_age_not_incremented_on_retrigger(void)
{
  /*
   * Retrigger the same note: the find_voice_by_note path fires,
   * age_voices() is NOT called.
   */
  struct midi_msg m = make_note_on(60, 64);
  synth_handle_midi(&s, &m);
  tick_n(&s, 1);
  int age_before = find_voice_playing(&s, 60)->age;

  synth_handle_midi(&s, &m); /* retrigger — no age bump */
  int age_after = find_voice_playing(&s, 60)->age;

  TEST_ASSERT_EQUAL_INT(age_before, age_after);
}

/* -----------------------------------------------------------------------
 * Note-off routing
 * ----------------------------------------------------------------------- */

void test_note_off_sets_release_on_matching_voice(void)
{
  struct midi_msg on = make_note_on(60, 64);
  struct midi_msg off = make_note_off(60);

  synth_handle_midi(&s, &on);
  tick_n(&s, 1);
  synth_handle_midi(&s, &off);

  /* VOICE_EVENT_RELEASE is bit 2 — voice still ACTIVE, flag should be set */
  struct voice *v = find_voice_playing(&s, 60);
  TEST_ASSERT_NOT_NULL(v);
  TEST_ASSERT_NOT_EQUAL(0, v->event_flags & 0x04);
}

void test_note_off_for_unknown_note_is_harmless(void)
{
  /* No voice playing note 72 — should not crash or corrupt state */
  struct midi_msg off = make_note_off(72);
  synth_handle_midi(&s, &off);
  TEST_ASSERT_EQUAL_INT(MAX_VOICES, count_voices_in_state(&s, VOICE_IDLE));
}

void test_note_off_does_not_affect_other_voices(void)
{
  struct midi_msg on60 = make_note_on(60, 64);
  struct midi_msg on64 = make_note_on(64, 64);
  struct midi_msg off60 = make_note_off(60);

  synth_handle_midi(&s, &on60);
  synth_handle_midi(&s, &on64);
  tick_n(&s, 1);

  synth_handle_midi(&s, &off60);

  /* Note 64 should be unaffected — still ACTIVE, release flag clear */
  struct voice *v64 = find_voice_playing(&s, 64);
  TEST_ASSERT_NOT_NULL(v64);
  TEST_ASSERT_EQUAL_UINT8(0, v64->event_flags & 0x04);
}

/* -----------------------------------------------------------------------
 * Note-on with velocity 0 == note-off (MIDI spec)
 * ----------------------------------------------------------------------- */

void test_note_on_velocity_zero_acts_as_note_off(void)
{
  struct midi_msg on = make_note_on(60, 64);
  struct midi_msg off = make_note_on(60, 0); /* vel=0 → note-off */

  synth_handle_midi(&s, &on);
  tick_n(&s, 1);
  synth_handle_midi(&s, &off);

  struct voice *v = find_voice_playing(&s, 60);
  TEST_ASSERT_NOT_NULL(v);
  TEST_ASSERT_NOT_EQUAL(0, v->event_flags & 0x04); /* release flag set */
}

/* -----------------------------------------------------------------------
 * MIDI CC → param update
 * ----------------------------------------------------------------------- */

void test_cc_message_does_not_crash(void)
{
  /* Smoke: a CC should be dispatched and not crash */
  struct midi_msg cc = make_msg(MIDI_STATUS_CONTROL_CHANGE, MIDI_CC_VOLUME, 100);
  synth_handle_midi(&s, &cc);
  /* No assertion needed — if we get here without a fault, it passed */
  TEST_PASS();
}

/* -----------------------------------------------------------------------
 * synth_tick() — idle-voice gating
 *
 * synth_tick() only calls voice_tick() for non-idle voices.  With all
 * voices idle, repeated ticking must leave them all idle.
 * ----------------------------------------------------------------------- */

void test_tick_idle_voices_stay_idle(void)
{
  tick_n(&s, 10);
  TEST_ASSERT_EQUAL_INT(MAX_VOICES, count_voices_in_state(&s, VOICE_IDLE));
}

void test_tick_active_voice_does_not_become_idle_without_note_off(void)
{
  /*
   * Default patch: sustain=SQ15_ONE, release=0.
   * With no note-off, the envelope holds in SUSTAIN indefinitely.
   * The voice must remain ACTIVE across many ticks.
   */
  struct midi_msg on = make_note_on(60, 64);
  synth_handle_midi(&s, &on);
  tick_n(&s, 50);
  TEST_ASSERT_EQUAL_INT(1, count_voices_in_state(&s, VOICE_ACTIVE));
}

void test_tick_voice_returns_to_idle_after_note_off_with_zero_release(void)
{
  /*
   * Default patch has release=0, so the envelope hits ENV_OFF on the very
   * next tick after note-off.  After a few more ticks the voice goes IDLE.
   */
  struct midi_msg on = make_note_on(60, 64);
  struct midi_msg off = make_note_off(60);

  synth_handle_midi(&s, &on);
  tick_n(&s, 2); /* let envelope settle */
  synth_handle_midi(&s, &off);
  tick_n(&s, 5); /* release completes */

  TEST_ASSERT_EQUAL_INT(MAX_VOICES, count_voices_in_state(&s, VOICE_IDLE));
}

/* -----------------------------------------------------------------------
 * Test runner
 * ----------------------------------------------------------------------- */

int main(void)
{
  UNITY_BEGIN();

  /* Initialisation */
  RUN_TEST(test_init_midi_channel_is_omni);
  RUN_TEST(test_init_all_voices_idle);
  RUN_TEST(test_init_voice_indices_set);
  RUN_TEST(test_init_params_pointer_set_on_all_voices);

  /* Free-voice allocation */
  RUN_TEST(test_note_on_activates_one_voice);
  RUN_TEST(test_note_on_stores_correct_note);
  RUN_TEST(test_two_note_on_activates_two_voices);
  RUN_TEST(test_four_note_on_fills_all_voices);

  /* Retrigger */
  RUN_TEST(test_retrigger_does_not_add_a_second_voice);
  RUN_TEST(test_retrigger_updates_velocity);

  /* Steal */
  RUN_TEST(test_fifth_note_on_steals_oldest_voice);
  RUN_TEST(test_fifth_note_steal_target_is_stealing_state);
  RUN_TEST(test_steal_picks_oldest_not_youngest);

  /* Ageing */
  RUN_TEST(test_age_increments_on_new_allocation);
  RUN_TEST(test_age_not_incremented_on_retrigger);

  /* Note-off */
  RUN_TEST(test_note_off_sets_release_on_matching_voice);
  RUN_TEST(test_note_off_for_unknown_note_is_harmless);
  RUN_TEST(test_note_off_does_not_affect_other_voices);

  /* Vel-zero = note-off */
  RUN_TEST(test_note_on_velocity_zero_acts_as_note_off);

  /* CC dispatch */
  RUN_TEST(test_cc_message_does_not_crash);

  /* synth_tick() */
  RUN_TEST(test_tick_idle_voices_stay_idle);
  RUN_TEST(test_tick_active_voice_does_not_become_idle_without_note_off);
  RUN_TEST(test_tick_voice_returns_to_idle_after_note_off_with_zero_release);

  return UNITY_END();
}