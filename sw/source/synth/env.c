/*
 *  (c) Jason Wilden, 2026
 */

#include "env.h"
#include "params.h"
#include "drv.h"

#ifdef TRACE_ENABLED
static const char *TRACE_FILE = "env.c";
#endif



/* State functions */
static void env_state_off(struct env *env);
static void env_state_attack(struct env *env);
static void env_state_decay(struct env *env);
static void env_state_sustain(struct env *env);
static void env_state_release(struct env *env);
static void env_state_shutdown(struct env *env);

/* Output transformations */
static SQ1_15 env_mode_normal(SQ1_15 level, SQ1_15 sustain);
static SQ1_15 env_mode_biased(SQ1_15 level, SQ1_15 sustain);
static SQ1_15 env_mode_inverted(SQ1_15 level, SQ1_15 sustain);
static SQ1_15 env_mode_biased_inverted(SQ1_15 level, SQ1_15 sustain);

/* State handler jump table */
static void (*env_state_handlers[ENV_MAX])(struct env *env) = {
    env_state_off, env_state_attack, env_state_decay,
    env_state_sustain, env_state_release, env_state_shutdown};

/* Output transform functions, jump table */
static SQ1_15 (*env_transforms[])(SQ1_15 level, SQ1_15 sustain) = {
    env_mode_normal, env_mode_biased, env_mode_inverted, env_mode_biased_inverted};


/*
 * Initialises the envelope.
 */    
void env_init(struct env *env)
{
  env_reset(env);
}

/*
 * Resets the envelope.
 */
void env_reset(struct env *env)
{
  env->state = ENV_OFF;
  env->level = 0;
}

/*
 * Processes one block of the envelope generator state machine
 *
 * Calls the appropriate state handler function directly using the jump
 * table indexed on envelope state.  It then applies a transform which
 * uses another function jump table indexed by the envelope mode.
 */
void env_render(struct env *env, SQ1_15 *output)
{
  env_state_handlers[env->state](env);
  *output = env_transforms[env->mode](env->level, env->sustain);
}


/*
 * Calc segment curves and start the envelope state machine
 *
 * The attack and decay are scaled by velocity/fcw if tracking is enabled, a higher
 * velocity/note reduces the attack/decay time as you might expect from a piano.  
 */
void env_note_on(struct env *env, uint8_t midi_note, uint8_t midi_velocity)
{
  // TODO : Implement scaling
  (void)midi_note;
  (void)midi_velocity;

  env->state = ENV_ATTACK;
}


/*
 * Triggers the release phase when a key is released
 * Transitions to RELEASE state if level is above zero, otherwise goes directly to OFF state.
 */

void env_note_off(struct env *env)
{
  if (env->level > 0)
  {
    env->state = ENV_RELEASE;
  }
  else
  {
    env->state = ENV_OFF;
  }
}

/*
 * Return To Zero - rapidly silences the envelope
 *
 * Used during voice stealing operations when a voice needs to be
 * quickly repurposed for a new note. Creates a short linear ramp
 * to zero regardless of current envelope state.
 */
void env_rtz(struct env *env)
{

  if (env->level > 0)
  {
    /* Minimum one block */
    const int32_t shutdown_blocks = 1;

    /* Q1.15 / int -> Q1.15, single block divide */
    env->inc_shutdown = (int16_t)(-(env->level / shutdown_blocks));
    env->state = ENV_SHUTDOWN;
  }
}

/*
 * Updates internal state following a parameter change. This uses a linear
 * mapping rather than a power curve for the parameter values.
 */
void env_update(struct env *env, uint8_t attack, uint8_t decay, SQ1_15 sustain, uint8_t release,
                uint8_t mode, bool note_track, bool velocity_track)
{
  env->attack  = attack;
  env->decay   = decay;
  env->sustain = sustain;
  env->release = release;

  env->mode = (mode < ENV_MODE_COUNT) ? mode : ENV_NORMAL;

  env->note_track     = note_track;
  env->velocity_track = velocity_track;

  env->attack_coeff  = env_attack_coeff_lut[env->attack];
  env->decay_coeff   = env_decay_coeff_lut[env->decay];
  env->release_coeff = env_decay_coeff_lut[env->release]; 

  env->attack_overshoot = (int16_t)env_attack_overshoot_lut[env->attack];
  env->release_overshoot = env_release_overshoot_lut[env->release];
  env->decay_overshoot = (int16_t)(env_decay_overshoot_base_lut[env->decay] +
                                    q15_mul((int16_t)env->sustain, (int16_t)(Q15_ONE - env->decay_coeff)));
}

/*
 * Implements the OFF state - the final envelope state where output is zero
 */
static void env_state_off(struct env *env)
{
  env->level = 0;
}


/*
 * Implements the ATTACK state - initial envelope segment with rising amplitude
 *
 * Automatically transitions to DECAY state when:
 *       - Level reaches 1.0 (full amplitude)
 *       - Attack time is zero (immediate attack)
 */
static void env_state_attack(struct env *env)
{
  if (env->attack == 0)
  {
    env->level = Q15_ONE;
    env->state = ENV_DECAY;
    return;
  }

  env->level = (Q1_15)(q15_mul((int16_t)env->level, (int16_t)env->attack_coeff) + env->attack_overshoot);

  if (env->level >= Q15_ONE)
  {
    env->level = Q15_ONE;
    env->state = ENV_DECAY;
  }
}

/*
 * Implements the DECAY state - segment where amplitude falls to sustain level
 *
 * Automatically transitions to SUSTAIN state when:
 *       - Level reaches the sustain level
 *       - Decay time is zero (immediate decay)
 *
 * If the decay is 0 then it jumps directly to the sustain.
 */
static void env_state_decay(struct env *env)
{
  if (env->decay == 0)
  {
    env->level = env->sustain;
    env->state = env->sustain > 0 ? ENV_SUSTAIN : ENV_RELEASE;
    return;
  }

  env->level = (Q1_15)(q15_mul((int16_t)env->level, (int16_t)env->decay_coeff) + env->decay_overshoot);

  if (env->level <= env->sustain)
  {
    env->level = env->sustain;
    env->state = env->sustain > 0 ? ENV_SUSTAIN : ENV_RELEASE;
  }
}

/*
 * Implements the SUSTAIN state - maintains constant level until note-off
 *
 * This state has no automatic transition - it remains active until explicitly
 * changed by a note-off event triggering the RELEASE state.
 */
static void env_state_sustain(struct env *env)
{
  env->level = env->sustain;
}

/*
 * Implements the RELEASE state - decreasing amplitude after note-off
 *
 * Triggered by note-off event, not by an automatic state transition.
 * Transitions to OFF state when amplitude reaches zero or release time is zero.
 */
static void env_state_release(struct env *env)
{
  if (env->release == 0)
  {
    env->level = 0;
    env->state = ENV_OFF;
    return;
  }

  env->level = (Q1_15)(q15_mul((int16_t)env->level, (int16_t)env->release_coeff) + env->release_overshoot);

  if ((int16_t)env->level <= 0)
  {
    env->level = 0;
    env->state = ENV_OFF;
  }
}

/*
 * Implements rapid amplitude fade to zero for voice termination
 *
 * Used during voice stealing or RTZ (return to zero) operations when
 * a voice needs to be quickly silenced. Uses linear ramp rather than
 */
static void env_state_shutdown(struct env *env)
{
  env->level = (Q1_15)((int16_t)env->level + env->inc_shutdown);

  if ((int16_t)env->level <= 0)
  {
    env->level = 0; /* Clear any overshoot caused by RTZ */
    env->state = ENV_OFF;
  }
}

/* output transformer, this is used for a normal envelope */
static SQ1_15 env_mode_normal(SQ1_15 level, SQ1_15 sustain)
{
  (void)sustain;
  return level;
}

/* output transformer, this is used for a biased envelope (usually fcw) */
static SQ1_15 env_mode_biased(SQ1_15 level, SQ1_15 sustain)
{
  return (SQ1_15)((int16_t)level - (int16_t)sustain);
}

/* output transformer, this is used for a inverted envelope */
static SQ1_15 env_mode_inverted(SQ1_15 level, SQ1_15 sustain)
{
  (void)sustain;
  return (SQ1_15)(SQ15_ONE - level);
}

/* output transformer, this is used for a biased and inverted envelope */
static SQ1_15 env_mode_biased_inverted(SQ1_15 level, SQ1_15 sustain)
{
  return (SQ1_15)((SQ15_ONE - level) - sustain);
}