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
  VOICE_EVENT_UPDATE = (1 << 3),
  VOICE_EVENT_STEAL_RTZ = (1 << 4),
  VOICE_EVENT_LEGATO = (1 << 5)
};

/* State functions */
static void voice_state_idle(struct voice *voice);
static void voice_state_active(struct voice *voice);
static void voice_state_stealing(struct voice *voice);

/* Modulators */
static void voice_init_modulators(struct voice *voice);
static void voice_start_modulators(struct voice *voice);
static void voice_stop_modulators(struct voice *voice);
static void voice_calculate_modulators(struct voice *voice);
static void voice_update_modulators(struct voice *voice);

/* State handler jump table */
static void (*voice_state_handlers[])(struct voice *voice) = {voice_state_idle,
                                                              voice_state_active,
                                                              voice_state_stealing};

void voice_init(struct voice *voice, uint8_t params[])
{
  voice->params = params;
  voice_reset(voice);
  voice_init_modulators(voice);
}

void voice_reset(struct voice *voice)
{
  voice->note = 0;
  voice->vel = 0;
  voice->pitch = 0;
  voice->steal_note = 0;
  voice->steal_vel = 0;
  voice->steal_pitch = 0;
  voice->state = VOICE_IDLE;
  voice->event_flags = VOICE_EVENT_NONE;
  voice->age = 0;
}

void voice_calculate(struct voice *voice)
{
  voice_state_handlers[voice->state](voice);
}

/*
 * Translates a MIDI note_on event into a VOICE_EVENT.
 *
 * MIDI events can be generated at any point in time by the player, they are asynchronous and occur
 * within the MIDI time domain.  We cannot process these immediately since the audio loop may be in
 * the middle of a cycle and any changes mid cycle could cause audible pops and clicks. Instead this
 * function derives and latches (stores) a VOICE_EVENT for later servicing at the correct point in
 * time.
 *
 * This temporal mapping moves the MIDI time domain event into the deterministic AUDIO time domain.
 *
 * The event it assigns is derived from the current state of the voice.
 *
 * - VOICE_EVENT_START     + State IDLE   : Free voice and new note, simplest case.
 * - VOICE_EVENT_RETRIGGER + State ACTIVE : The note is the same, retrigger envelope.
 * - VOICE_EVENT_STEAL_RTZ + State ACTIVE : The note is different, initiate the voice steal process.
 * - VOICE_EVENT_UPDATE                   : A parameter has changed, update the signal chain
 * modules.
 *
 * Voice updates must be processed for all states, updating the internal state of the signal chain
 * can happen at any point in a voice lifecycle and needs to be scheduled just like the note events.
 *
 * Note that we should never see a note_on event arrive when the voice is in the VOICE_STEALING
 * state, our voice algorithm does not steal voices that are in mid-steal already, so this is a
 * fault and we should assert in debug mode.
 */
void voice_note_on(struct voice *voice, uint8_t midi_note, uint8_t midi_velocity)
{
  TRACE_ASSERT(voice);

  switch (voice->state)
  {
  case VOICE_IDLE:

    // TRACE_PRINTF("Note On: Request VOICE_EVENT_START [voice %d]\n", voice->idx,0,0);

    voice->note = midi_note;
    voice->vel = midi_velocity;

    // TODO: NYI
    // voice->pitch = MIDI_FREQ_TABLE[midi_note];
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

    // TODO: NYI
    // voice->steal_pitch = MIDI_FREQ_TABLE[midi_note];
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
 *
 * See the comments for voice_note_on() for more details, this function does
 * the same job for the note_off MIDI event.
 *
 * The only case it needs to handle is:
 * - VOICE_EVENT_RELEASE     + State ACTIVE   : Indicates that the voice should be released.
 *
 * The synth voice allocation algorithm should ensure that we never receive a note off for a
 * voice that is not active so we assert if that happens.
 */
void voice_note_off(struct voice *voice, uint8_t midi_note)
{    
  TRACE_ASSERT(voice);
  // TRACE_PRINT_DEC("NoteOn:VOICE_EVENT_RELEASE:",voice->idx);
  voice->event_flags |= VOICE_EVENT_RELEASE;
}

/* brief translates an update request into a VOICE_EVENT.
 *
 * Updates apply regardless of the voice state. VOICE_ACTIVE obviously needs to update. VOICE_IDLE
 * so that when the voice is activated it has the updated sound. VOICE_STEALING, the stolen
 * voice and stealing voice should reflect the updated params although realistically the change to
 * the voice being stolen is unlikely to be heard during the quick shutdown (RTZ).
 *
 * Like the note_on/note_off functions, it ensure the event is latched for handling at the start of
 * next render cycle.
 *
 */
void voice_update(struct voice *voice)
{
   voice->event_flags |= VOICE_EVENT_UPDATE;
}

/*
 * Handles the VOICE_IDLE state
 *
 * The only event that needs to be serviced while the voice is idle is the VOICE_EVENT_UPDATE which
 * indicates a global parameter (synth user control) has changed.
 *
 * VOICE_EVENT_UPDATE: Updates the parameters for the modulators.
 *
 */
static void voice_state_idle(struct voice *voice)
{
   if (voice->event_flags & VOICE_EVENT_UPDATE)
  {    
    // TRACE_PRINT_DEC("NoteOn:VOICE_UPDATE(IDLE):",voice->idx);
    voice_update_modulators(voice);
    voice->event_flags &= ~VOICE_EVENT_UPDATE;
  }
}

/*
 * Handles the VOICE_ACTIVE state
 *
 * When the voice is active, any events need to be handled before the signal chain is called
 * so that they take effect at the start of the block.  This is where the asynchronous MIDI
 * time-domain events are converted to deterministic AUDIO time domain actions.
 *
 * VOICE_EVENT_START : Starts the signal chain for a new note.
 * VOICE_EVENT_RETRIGGER : Resets the envelope for the ACTIVE voice.
 * VOICE_EVENT_RELEASE:  Moves the envelope generator to the release segment.
 * VOICE_EVENT_UPDATE: Updates the parameters for the signal chain modules.
 * VOICE_EVENT_LEGATO: In mono mode only, notify a note on without changing envelope.
 * ENV_OFF: Switches the voice to IDLE.
 *
 * Once any events have been serviced, and there might be none, render is run to render the
 * modulator values.
 */
static void voice_state_active(struct voice *voice)
{
  if (voice->event_flags & VOICE_EVENT_UPDATE)
  {    
    voice_update_modulators(voice);
    voice->event_flags &= ~VOICE_EVENT_UPDATE;
  }

  if (voice->event_flags & VOICE_EVENT_START)
  {   
    /* New note, start the modulators, set envelopes to ATTACK state */
    voice_start_modulators(voice);

    // TODO: NYI
    //env_note_on(&voice->amp_env, voice->note, voice->velocity);

    voice->event_flags &= ~VOICE_EVENT_START;
  }

  if (voice->event_flags & VOICE_EVENT_RETRIGGER)
  {
    /* RTT_LOG("%sRender: Service VOICE_EVENT_RETRIGGER [voice %d]\n", RTT_CTRL_TEXT_BRIGHT_GREEN,
            voice->idx); */

    /* Retrigger by setting the envelope to ATTACK state */
    // TODO: NYI
    // env_note_on(&voice->amp_env, voice->note, voice->velocity);

    voice->event_flags &= ~VOICE_EVENT_RETRIGGER;
  }

  if (voice->event_flags & VOICE_EVENT_RELEASE)
  {
    /* RTT_LOG("%sRender: Service VOICE_EVENT_RELEASE [voice %d]\n", RTT_CTRL_TEXT_BRIGHT_GREEN,
            voice->idx); */

    /* End the note by setting the envelope to RELEASE state, note that we are not ending the
     * signal chain here as the voice may have a release phase and need to 'ring out'. We handle
     * the ENV_OFF state as a separate state */
    // TODO: NYI
    // env_note_off(&voice->amp_env);
    voice->event_flags &= ~VOICE_EVENT_RELEASE;
  }

  // TODO: NYI
  // if (voice->amp_env.state == ENV_OFF)
  // {
  //   /* RTT_LOG("%sRender: Service ENV_OFF [voice %d]\n", RTT_CTRL_TEXT_BRIGHT_GREEN, voice->idx);
  //    */
  //   /* Envelope has ended so note is now silent, stop the signal chain running and set the voice to
  //    * IDLE */
  //   RTT_LOG("Voice %d: ENV_OFF -> IDLE transition\n", voice->idx);
  //   voice_stop_chain(voice);    RTT_LOG("Voice %d: ENV_OFF -> IDLE transition\n", voice->idx);
  //   voice->state = VOICE_IDLE;
  //   voice->event_flags = VOICE_EVENT_NONE;

  //   /* We return directly here as there is no need to run the signal chain, the voice is silent */
  //   return;
  // }

  /* Any events have been serviced, run the signal chain to generate samples */
  voice_calculate_modulators(voice);
}

/*
 * Services the voice steal event
 *
 * Again the goal here is to convert the ASYNC MIDI event into a deterministic AUDIO event.
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
 * We still handle voice updates in this state, the audio signal chain is running during this
 * state and we need to reflect user changes to parameters as usual.
 *
 * VOICE_EVENT_STEAL_RTZ: Signal the enveloper generator to quickly fade the voice to silence.
 * ENV_OFF: Voice is silent, safe to swap in the new note and set to VOICE_ACTIVE.
 */
static void voice_state_stealing(struct voice *voice)
{
   if (voice->event_flags & VOICE_EVENT_UPDATE)
  {
    voice_update_modulators(voice);
    voice->event_flags &= ~VOICE_EVENT_UPDATE;
  }

  if (voice->event_flags & VOICE_EVENT_STEAL_RTZ)
  {
    /* RTT_LOG("%sRender: Service VOICE_EVENT_STEAL_RTZ [voice %d]\n", RTT_CTRL_TEXT_BRIGHT_GREEN,
            voice->idx); */
    // TODO: NYI
    // env_rtz(&voice->amp_env);
    voice->event_flags &= ~VOICE_EVENT_STEAL_RTZ;
  }

  // TODO: NYI
  // if (voice->amp_env.state == ENV_OFF)
  // {
  //   /* RTT_LOG("%sRender: Service ENV_OFF(RTZ) [voice %d]\n", RTT_CTRL_TEXT_BRIGHT_GREEN,
  //    * voice->idx); */
  //   voice->note = voice->steal_note;
  //   voice->velocity = voice->steal_velocity;
  //   voice->pitch = voice->steal_pitch;
  //   voice->event_flags |= VOICE_EVENT_START;
  //   voice->state = VOICE_ACTIVE;

  //   /* We leave the signal chain running as we know we're going straight into the steal note */
  // }

  /* Events have been serviced, run the modulators to generate values */
  voice_calculate_modulators(voice);
}

/*
 * Initialises the state of the modulators.
 */
static void voice_init_modulators(struct voice *voice)
{
  env_init(&voice->amp_env);
  env_init(&voice->mod_env);
}

/*
 * Calculates the current modulator values.
 *
 * This is called by the voice_render function.
 */
static void voice_calculate_modulators(struct voice *voice)
{
  // env_render(&voice->amp_env, &voice->modulators[ENV1_LEVEL]);
}

/*
 * Updates the voice modulators.
 *
 * We do not try to determine which parameters have changed and limit what modules update
 * in any way.  We only have around 35 voice parameter and most of these are
 * computationally lightweight.  The update is called only when a CC message is
 * mapped so not in the voice render cycle.
 *
 * For a larger set of parameters we would use a finer level of granularity by
 * dividing the param array into segments where each holds the mappings for a
 * particular module, we mark the end of the array identifiers with a special
 * value - say OSC_COUNT, AMP_ENV_COUNT etc and would identify any mapping in
 * a 0-OSC_COUNT as being for oscillators, OSC_COUNT to AMP_ENV_COUNT as being
 * the amp envelope and so on.  This is an extension of how the VOICE and
 * SYNTH division in the mappings is handled already.
 */
static void voice_update_modulators(struct voice *voice)
{
  // TODO: NYI
  // Cascaded update calls here
}

void voice_start_modulators(struct voice *voice)
{
  /* RTT_LOG("%sStart voice chain %d\n", RTT_CTRL_TEXT_WHITE, voice->idx); */
  // TODO:   NYI
  // lfo_note_on(&voice->lfo1);
}

/**
 * \brief stops the signal chain
 *
 * Not all modules require this notification, only those with a matching
 * note_on call will have a note_off.
 */
void voice_stop_modulators(struct voice *voice)
{
  /* RTT_LOG("%sStop voice chain %d\n", RTT_CTRL_TEXT_WHITE, voice->idx); */  
}

