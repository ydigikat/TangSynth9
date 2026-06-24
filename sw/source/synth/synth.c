
/*
 *  (c) Jason Wilden, 2026
 */

#include "synth.h"
#include "params.h"
#include "drv.h"

#ifdef TRACE_ENABLED
static const char *TRACE_FILE = "synth.c";
#endif

/* Number of samples in each control-rate cycle */
#define LAST_IRQ_IN_CYCLE (46U)

static struct midi_instance midi_in;

/* Private functions */
static void note_on(struct synth *synth, uint8_t note, uint8_t velocity);
static void note_off(struct synth *synth, uint8_t note);
static void update_voice_params(struct synth *synth);
static struct voice *find_oldest_active_voice(struct synth *synth);
static struct voice *find_voice_by_note(struct synth *synth, uint8_t note);
static inline struct voice *find_free_voice(struct synth *synth);
static inline void age_voices(struct synth *synth);

void synth_init(struct synth *synth)
{
  synth->midi_channel = MIDI_OMNI;

  param_init();
  param_create_default_patch(synth->params);

  for (int i = 0; i < MAX_VOICES; i++)
  {
    synth->voice[i].idx = i;
    voice_init(&synth->voice[i], synth->params);
    voice_update(&synth->voice[i]);
  }
}

/*
 * Dispatch MIDI messages
 */
void synth_handle_midi(struct synth *synth, struct midi_msg *msg)
{
  TRACE_ASSERT(synth);
  TRACE_ASSERT(msg);

  switch (msg->data[0])
  {
  case MIDI_STATUS_NOTE_ON:
    if (msg->data[2] > 0)
    {
      // TRACE_PRINT_DEC("NoteOn:",msg->data[1]);
      note_on(synth, msg->data[1], msg->data[2]);
    }
    else
    {
      /* Controllers can send a NOTE_ON with velocity 0 instead of NOTE_OFF */
      // TRACE_PRINT_DEC("NoteOff:",msg->data[1]);
      note_off(synth, msg->data[1]);
    }

    break;

  case MIDI_STATUS_NOTE_OFF:
    // TRACE_PRINT_DEC("NoteOff:",msg->data[1]);
    note_off(synth, msg->data[1]);
    break;

  case MIDI_STATUS_CONTROL_CHANGE:
  {
    // TRACE_PRINTF("Controller: %d, %d\n", msg->data[1], msg->data[2], 0);

    update_voice_params(synth);
  }
  break;
  }
}

/*
 * Calls the voices to update their internal calculations for modulators.
 */
void synth_render(struct synth *synth)
{
#pragma GCC unroll 8
  for (int i = 0; i < MAX_VOICES; i++)
  {
    voice_calculate(&synth->voice[i]);
  }
}

/*
 * Executes the control-rate calculation cycle by counting the interrupts.
 */
uint8_t synth_execute(struct synth *synth, uint8_t irq_count)
{
  TRACE_ASSERT(synth);

  if (irq_count == 1U)
  {
    /*
     * On the first interrupt of a cycle we:
     *
     * - set the APCR register to stop the audio pipeline updating from VRAM
     * - parse any buffered MIDI events
     * - process all the voice calculations (modulators & lifecycle)
     * - copy the new calculated values into VRAM.
     */

    CLEAR_BIT(APCR->CR, APCR_CR_VRAM_UPDATE);

    uint8_t byte;
    while (midi_buffer_read(&byte))
    {
      struct midi_msg *msg = midi_parse(&midi_in, byte);
      if (msg != NULL)
      {
        // TRACE_PRINT_HEX("D0:",msg->data[0],2);
        // TRACE_PRINT_HEX("D1:",msg->data[1],2);
        // TRACE_PRINT_HEX("D2:",msg->data[2],2);
        synth_handle_midi(synth, msg);
      }
    }

    synth_render(synth);

    // TODO: Copy to VRAM here
  }
  else if (irq_count == LAST_IRQ_IN_CYCLE)
  {
    /*
     * On the last interrupt of the cycle we:
     *
     * - set the APCR register so the pipeline updates before next sample
     * - reset the sample counter.
     */
    SET_BIT(APCR->CR, APCR_CR_VRAM_UPDATE);
    irq_count = 1;
  }

  return irq_count;
}

/*
 * Allocate and trigger a voice for the incoming note.
 *
 * Allocation strategy:
 *   1. Voice already playing this note (retrigger)
 *   2. Free (idle) voice
 *   3. Oldest active voice (steal)
 *
 * Voice age tracking: When allocating a new or stolen voice, all active voices
 * age by one generation. This relative lifetime counter identifies the longest-
 * sounding voice for stealing. Age resets to 0 when a voice ends.
 */
static void note_on(struct synth *synth, uint8_t note, uint8_t velocity)
{
  struct voice *voice = NULL;

  voice = find_voice_by_note(synth, note);
  if (voice)
  {
    voice_note_on(voice, note, velocity);
    return;
  }

  voice = find_free_voice(synth);
  if (voice)
  {
    age_voices(synth);
    voice_note_on(voice, note, velocity);
    return;
  }

  voice = find_oldest_active_voice(synth);
  if (voice)
  {
    age_voices(synth);
    voice_note_on(voice, note, velocity);
    return;
  }
}

/*
 * Release a voice when its key is released.
 * this direct way as well. this direct way as well.
 *
 *   Locate and release the voice playing this note. May not find one - if the
 *   voice was stolen while the key was held, it's already playing a different note.
 */
static void note_off(struct synth *synth, uint8_t note)
{
  struct voice *voice = find_voice_by_note(synth, note);
  if (voice)
  {
    voice_note_off(voice);
  }
}

/*
 * Update the voice parameters.
 *
 * Call each voice to update their internal state and those of the signal
 * chain directly. The voice manages voice parameter updates entirely.
 */
void update_voice_params(struct synth *synth)
{
#pragma GCC unroll 8
  for (int i = 0; i < MAX_VOICES; i++)
  {
    voice_update(&synth->voice[i]);
  }
}

/*
 * Find an idle voice ready for immediate use
 */
static inline struct voice *find_free_voice(struct synth *synth)
{
  struct voice *free_voice = NULL;

  for (int i = 0; i < MAX_VOICES; i++)
  {
    if (synth->voice[i].state == VOICE_IDLE)
    {
      return &synth->voice[i];
    }
  }
  return free_voice;
}

/*
 * Find the longest-running active voice for stealing.
 * Only considers ACTIVE voices - voices mid-release are excluded.
 */
static inline struct voice *find_oldest_active_voice(struct synth *synth)
{
  int8_t age = -1;
  struct voice *oldest_voice = NULL;

  for (int i = 0; i < MAX_VOICES; i++)
  {
    if (synth->voice[i].state == VOICE_ACTIVE && synth->voice[i].age > age)
    {
      age = synth->voice[i].age;
      oldest_voice = &synth->voice[i];
    }
  }

  return oldest_voice;
}

/*
 * Find which active voice is currently playing a specific note.
 * Returns NULL if not found - may occur if voice was stolen while key held.
 */
static inline struct voice *find_voice_by_note(struct synth *synth, uint8_t note)
{
  for (int i = 0; i < MAX_VOICES; i++)
  {
    if (synth->voice[i].state == VOICE_ACTIVE && synth->voice[i].note == note)
    {
      return &synth->voice[i];
    }
  }

  return NULL;
}

/*
 * Advance the age of all active voices by one generation.
 *
 * Voice age is a relative lifetime counter for voice stealing. Active voices
 * increment their age once per generation; voices reset to 0 when they end.
 * The highest age always identifies the longest-sounding voice.
 */
static inline void age_voices(struct synth *synth)
{
  for (int i = 0; i < MAX_VOICES; i++)
  {
    if (synth->voice[i].state == VOICE_ACTIVE)
    {
      synth->voice[i].age++;
    }
  }
}
