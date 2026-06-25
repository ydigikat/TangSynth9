/*
 *  (c) Jason Wilden, 2026
 */

#include "voice.h"
#include "drv.h"

#ifdef TRACE_ENABLED
static const char *TRACE_FILE = "voice.c";
#endif

enum VOICE_EVENT
{
  VOICE_EVENT_NONE = 0,
  VOICE_EVENT_START = (1 << 0),
  VOICE_EVENT_RETRIGGER = (1 << 1),
  VOICE_EVENT_RELEASE = (1 << 2),
  VOICE_EVENT_STEAL_RTZ = (1 << 3)
};

/* State functions */
static void voice_state_idle(struct voice *voice);
static void voice_state_active(struct voice *voice);
static void voice_state_stealing(struct voice *voice);

/* Modulators */
static void voice_run_modulators(struct voice *voice);

/* State handler jump table */
static void (*voice_state_handlers[])(struct voice *voice) = {voice_state_idle,
                                                              voice_state_active,
                                                              voice_state_stealing};

void voice_init(struct voice *voice, const param_value_t *restrict params)
{
  // TRACE_ASSERT(voice);
  // TRACE_ASSERT(params);

  voice->params = params;
  voice_reset(voice);

  env_init(&voice->amp_env);
  env_init(&voice->mod_env);
  lfo_init(&voice->lfo1);  
}

void voice_reset(struct voice *voice)
{
  // TRACE_ASSERT(voice);

  voice->note = 0;
  voice->vel = 0;
  voice->fcw = 0;
  voice->steal_note = 0;
  voice->steal_vel = 0;
  voice->steal_fcw = 0;
  voice->state = VOICE_IDLE;
  voice->event_flags = VOICE_EVENT_NONE;
  voice->age = 0;
}

void voice_tick(struct voice *voice)
{
  // TRACE_ASSERT(voice);
  voice_state_handlers[voice->state](voice);
}

/*
 * Translates a MIDI note_on event into a VOICE_EVENT.
 *
 * The event type it assigns derives from the current state of the voice.
 *
 * - VOICE_EVENT_START     + State IDLE   : Free voice and new note, simplest case.
 * - VOICE_EVENT_RETRIGGER + State ACTIVE : The note is the same, retrigger envelope.
 * - VOICE_EVENT_STEAL_RTZ + State ACTIVE : The note is different, initiate the voice steal process. 
 *
 * Note that we should never see a note_on event arrive when the voice is in the VOICE_STEALING
 * state, our voice algorithm does not steal voices that are in mid-steal already, so this is a
 * fault and we should assert in debug mode.
 */
void voice_note_on(struct voice *voice, uint8_t midi_note, uint8_t midi_velocity)
{
  // TRACE_ASSERT(voice);

  switch (voice->state)
  {
  case VOICE_IDLE:

    // TRACE_PRINTF("Note On: Request VOICE_EVENT_START [voice %d]\n", voice->idx,0,0);

    voice->note = midi_note;
    voice->vel = midi_velocity;
    voice->fcw = midi_fcw_lut[midi_note];
    voice->event_flags |= VOICE_EVENT_START;
    voice->state = VOICE_ACTIVE;
    break;

  case VOICE_ACTIVE:
    if (voice->note == midi_note)
    {
      // TRACE_PRINT_DEC("NoteOn:VOICE_EVENT_RETRIGGER:",voice->idx);

      voice->vel = midi_velocity;
      voice->event_flags |= VOICE_EVENT_RETRIGGER;
      return;
    }

    // TRACE_PRINT_DEC("NoteOn:VOICE_EVENT_STEAL_RTZ:",voice->idx);
    voice->steal_fcw = midi_fcw_lut[midi_note];
    voice->steal_note = midi_note;
    voice->steal_vel = midi_velocity;
    voice->state = VOICE_STEALING;
    voice->event_flags |= VOICE_EVENT_STEAL_RTZ;
    break;

  case VOICE_STEALING:
    //    TRACE_ASSERT(false); /* this should not happen */
    break;
  }
}

/*
 * Translates a note_off MIDI event into a VOICE_EVENT.
 **
 * The only case it needs to handle is:
 * - VOICE_EVENT_RELEASE     + State ACTIVE   : Indicates that the voice should be released.
 *
 * The synth voice allocation algorithm should ensure that we never receive a note off for a
 * voice that is not active so we assert if that happens.
 */
void voice_note_off(struct voice *voice)
{
  // TRACE_ASSERT(voice);
  // TRACE_PRINT_DEC("NoteOn:VOICE_EVENT_RELEASE:",voice->idx);
  voice->event_flags |= VOICE_EVENT_RELEASE;
}

/*
 * Updates apply regardless of the voice state. VOICE_ACTIVE obviously needs to update. VOICE_IDLE
 * so that when the voice is activated it has the updated sound. VOICE_STEALING, the stolen
 * voice and stealing voice should reflect the updated params although realistically the change to
 * the voice being stolen is unlikely to be heard during the quick shutdown (RTZ).
 */
void voice_update(struct voice *voice)
{
  env_update(&voice->amp_env, voice->params[AMP_ATTACK], voice->params[AMP_DECAY],
            voice->params[AMP_SUSTAIN], voice->params[AMP_RELEASE], ENV_NORMAL,
             voice->params[AMP_NOTE_TRACK], voice->params[AMP_VEL_TRACK]);

  env_update(&voice->mod_env, voice->params[ENV1_ATTACK], voice->params[ENV1_DECAY],
             voice->params[ENV1_SUSTAIN], voice->params[ENV1_RELEASE], voice->params[ENV1_MODE],
             voice->params[ENV1_NOTE_TRACK], voice->params[ENV1_VEL_TRACK]);

  lfo_update(&voice->lfo1, voice->params[LFO1_RATE], voice->params[LFO1_MODE]);
}

/*
 * Handles the VOICE_IDLE state (nothing to do, gated by synth_tick)
 */
static void voice_state_idle(struct voice *voice)
{  
}

/*
 * Handles the VOICE_ACTIVE state
 *
 * When the voice is active, any lifecycle events need to be handled before the 
 * modulator chain is called so that they take effect at the start of the block.
 *
 * VOICE_EVENT_START : Starts the modulators for a new note.
 * VOICE_EVENT_RETRIGGER : Resets the envelope for the ACTIVE voice.
 * VOICE_EVENT_RELEASE:  Moves the envelope generator to the release segment.
 * VOICE_EVENT_LEGATO: In mono mode only, notify a note on without changing envelope.
 * ENV_OFF: Switches the voice to IDLE.
 *
 * Once any events have been serviced, and there might be none, render is run.
 */
static void voice_state_active(struct voice *voice)
{  
  if (voice->event_flags & VOICE_EVENT_START)
  {
    /* New note, start the LFO, set envelopes to ATTACK state */
    lfo_note_on(&voice->lfo1);
    env_note_on(&voice->amp_env, voice->note, voice->vel);

    voice->event_flags &= ~VOICE_EVENT_START;
  }

  if (voice->event_flags & VOICE_EVENT_RETRIGGER)
  {
    // TRACE_PRINT_DEC("VOICE_EVENT_RETRIGGER:",voice->idx);
    /* Retrigger by setting the envelope to ATTACK state */
    env_note_on(&voice->amp_env, voice->note, voice->vel);

    voice->event_flags &= ~VOICE_EVENT_RETRIGGER;
  }

  if (voice->event_flags & VOICE_EVENT_RELEASE)
  {
    // TRACE_PRINT_DEC("VOICE_EVENT_RELEASE:",voice->idx);

    /*
     * End the note by setting the envelope to RELEASE state, note that we are not ending the
     * modulators here as the voice may have a release phase and need to 'ring out'. We handle
     * the ENV_OFF state as a separate state
     */

    env_note_off(&voice->amp_env);
    voice->event_flags &= ~VOICE_EVENT_RELEASE;
  }

  if (voice->amp_env.state == ENV_OFF)
  {
    // TRACE_PRINT_DEC("ENV_OFF->IDLE:",voice->idx);
    voice->state = VOICE_IDLE;
    voice->event_flags = VOICE_EVENT_NONE;

    /* We return directly here as there is no need to run the modulators anymore, the voice is silent */
    return;
  }

  /* Any events have been serviced, run the modulators to generate samples */
  voice_run_modulators(voice);
}

/*
 * Services the voice steal event
 *
 * We steal the voice in 2 distinct phases:
 *
 * Phase 1. We fade the current note quickly to zero (RTZ)
 * Phase 2. Once silent (ENV_OFF) we swap the steal note in and reset the voice to VOICE_ACTIVE.
 *
 * This 2-stage approach stops the note from abruptly changing mid-envelope which would cause
 * audible artifacts. The special state VOICE_STEALING is used to inform the synth voice
 * allocator that the voice is mid-steal and cannot be stolen again until the transition is
 * complete.
 *
 * We still handle voice updates in this state, the audio modulators is running during this
 * state and we need to reflect user changes to parameters as usual.
 *
 * VOICE_EVENT_STEAL_RTZ: Signal the enveloper generator to quickly fade the voice to silence.
 * ENV_OFF: Voice is silent, safe to swap in the new note and set to VOICE_ACTIVE.
 */
static void voice_state_stealing(struct voice *voice)
{  
  if (voice->event_flags & VOICE_EVENT_STEAL_RTZ)
  {
    // TRACE_PRINT_DEC("VOICE_EVENT_STEAL_RTZ:",voice->idx);
    env_rtz(&voice->amp_env);
    voice->event_flags &= ~VOICE_EVENT_STEAL_RTZ;
  }

  if (voice->amp_env.state == ENV_OFF)
  {
    // TRACE_PRINT_DEC("ENV_OFF(RTZ):",voice->idx);
    voice->note = voice->steal_note;
    voice->vel = voice->steal_vel;
    voice->fcw = voice->steal_fcw;
    voice->event_flags |= VOICE_EVENT_START;
    voice->state = VOICE_ACTIVE;

    /* We leave the modulators running as we know we're going straight into the steal note */
  }

  /* Events have been serviced, run the modulators to generate values */
  voice_run_modulators(voice);
}


/*
 * Calculates the current modulator values.
 *
 * This is called by the voice_tick function.
 */
static void voice_run_modulators(struct voice *voice)
{

  env_render(&voice->amp_env, &voice->mod_value[AMP_LEVEL]);
  env_render(&voice->mod_env, &voice->mod_value[ENV1_LEVEL]);
  lfo_render(&voice->lfo1, &voice->mod_value[LFO1_TRI], &voice->mod_value[LFO1_SAW],
             &voice->mod_value[LFO1_RAMP], &voice->mod_value[LFO1_SQUARE],
             &voice->mod_value[LFO1_SANDH]);
}
