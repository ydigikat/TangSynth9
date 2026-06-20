/*
 *  (c) Jason Wilden, 2026
 */

#include "env.h"

/* State functions */
static void env_state_off(struct env *env);
static void env_state_attack(struct env *env);
static void env_state_decay(struct env *env);
static void env_state_sustain(struct env *env);
static void env_state_release(struct env *env);
static void env_state_shutdown(struct env *env);

/* Output transformations */
static Q1_15 env_mode_normal(Q1_15 level, Q1_15 sustain);
static Q1_15 env_mode_biased(Q1_15 level, Q1_15 sustain);
static Q1_15 env_mode_inverted(Q1_15 level, Q1_15 sustain);
static Q1_15 env_mode_biased_inverted(Q1_15 level, Q1_15 sustain);

/* State handler jump table */
static void (*env_state_handlers[ENV_MAX])(struct env *env) = {
    env_state_off, env_state_attack, env_state_decay,
    env_state_sustain, env_state_release, env_state_shutdown};

/* Output transform functions, jump table */
static Q1_15 (*env_transforms[])(Q1_15 level, Q1_15 sustain) = {
    env_mode_normal, env_mode_biased, env_mode_inverted, env_mode_biased_inverted};

void env_init(struct env *env)
{
  env_reset(env);
}

void env_reset(struct env *env)
{
  env->state = ENV_OFF;
  env->level = 0;
}

void env_render(struct env *env, Q1_15 *output)
{
  env_state_handlers[env->state](env);
  *output = env_transforms[env->mode](env->level, env->sustain);
}

void env_note_on(struct env *env, uint8_t midi_note, uint8_t midi_velocity)
{
  // TODO : Scalers not implemented yet.
  (void)midi_note;
  (void)midi_velocity;

  env->state = ENV_ATTACK;
}

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

void env_update(struct env *env, uint8_t attack, uint8_t decay, Q1_15 sustain, uint8_t release,
                uint8_t mode, bool note_track, bool velocity_track)
{
  env->attack  = attack;
  env->decay   = decay;
  env->sustain = sustain;
  env->release = release;

  env->mode = (mode < ENV_MODE_COUNT) ? mode : ENV_MODE_NORMAL;

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

static void env_state_off(struct env *env)
{
  env->level = 0;
}

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

static void env_state_sustain(struct env *env)
{
  env->level = env->sustain;
}

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
static Q1_15 env_mode_normal(Q1_15 level, Q1_15 sustain)
{
  (void)sustain;
  return level;
}

/* output transformer, this is used for a biased envelope (usually pitch) */
static Q1_15 env_mode_biased(Q1_15 level, Q1_15 sustain)
{
  return (Q1_15)((int16_t)level - (int16_t)sustain);
}

/* output transformer, this is used for a inverted envelope */
static Q1_15 env_mode_inverted(Q1_15 level, Q1_15 sustain)
{
  (void)sustain;
  return (Q1_15)(Q15_ONE - level);
}

/* output transformer, this is used for a biased and inverted envelope */
static Q1_15 env_mode_biased_inverted(Q1_15 level, Q1_15 sustain)
{
  return (Q1_15)((Q15_ONE - level) - sustain);
}