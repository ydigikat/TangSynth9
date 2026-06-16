/*
 *  (c) Jason Wilden, 2026
 */

#include "synth.h"
#include "drv.h"

/* Private functions */
static void synth_note_on(struct synth *synth, uint8_t note, uint8_t velocity);
static void synth_note_off(struct synth *synth, uint8_t note);
static void synth_update_voice_params(struct synth *synth);
static struct voice *find_oldest_active_voice(struct synth *synth);
static struct voice *find_voice_by_note(struct synth *synth, uint8_t note);
static inline struct voice *find_free_voice(struct synth *synth);
static inline void age_voices(struct synth *synth);

void synth_init(struct synth *synth)
{
  synth->midi_channel = MIDI_OMNI;

  for (int i = 0; i < MAX_VOICES; i++)
  {
    synth->voice[i].idx = i;
    voice_init(&synth->voice[i], synth->params);
    voice_update(&synth->voice[i]);
  }

  trace_printf(TRACE, "Synth:Initialised. MIDI channel %x\n", synth->midi_channel, 0, 0);
}

/*
* Dispatch MIDI messages
*/
void synth_handle_midi(struct synth *synth, struct midi_msg *msg)
{
  switch (msg->data[0])
  {
  case MIDI_STATUS_NOTE_ON:
    if (msg->data[2] > 0)
    {
      trace_printf(TRACE,"Synth:Note On %x, %x\n", msg->data[1], msg->data[2],0);      
      synth_note_on(synth,msg->data[1], msg->data[2]);
    }
    else
    {
      /* Controllers can send a NOTE_ON with velocity 0 instead of NOTE_OFF */
      trace_printf(TRACE,"Synth:Note On (zero velocity) %x\n", msg->data[1],0,0);            
      synth_note_off(synth,msg->data[1]);
    }
    
    break;

  case MIDI_STATUS_NOTE_OFF:
    trace_printf(TRACE,"Synth:Note Off (zero velocity) %x\n", msg->data[1],0,0);                
    synth_note_off(synth,msg->data[1]);
    break;

  case MIDI_STATUS_CONTROL_CHANGE:
  {    
    trace_printf(TRACE,"Synth: %d, %d\n", msg->data[1],msg->data[2],0);            
    
    synth_update_voice_params(synth);
  }
  break;
  }
}

/*
* Calls the voices to update their internal calculations for modulators.
*/
void synth_calculate(struct synth *synth)
{
  #pragma GCC unroll 8
  for (int i = 0; i < MAX_VOICES; i++)
  {
    voice_calculate(&synth->voice[i]);
  }
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
static void synth_note_on(struct synth *synth, uint8_t note, uint8_t velocity)
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
static void synth_note_off(struct synth *synth, uint8_t note)
{
  struct voice *voice = find_voice_by_note(synth, note);
  if (voice)
  {
    voice_note_off(voice, note);
  }
}

/*
 * Update the voice parameters.
 *
 * Call each voice to update their internal state and those of the signal
 * chain directly. The voice manages voice parameter updates entirely.
 */
void synth_update_voice_params(struct synth *synth)
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