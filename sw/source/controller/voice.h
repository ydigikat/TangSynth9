/*
 *  (c) Jason Wilden, 2026
 */

#ifndef __VOICE_H__
#define __VOICE_H__

#include <stdint.h>
#include "types.h"
#include "params.h"
#include "env.h"

enum voice_state
{
  VOICE_IDLE,
  VOICE_ACTIVE,
  VOICE_STEALING
};

struct voice
{
  /* State */
  enum voice_state state;
  uint8_t event_flags;
  uint8_t idx;
  int8_t age;
  uint8_t note, steal_note;
  uint8_t vel, steal_vel;

  /* Parameters */
  const param_value_t *params;

  /* Normalised values */
  Q1_15 pitch, steal_pitch;
  Q1_15 amp_eg_lvl;
  Q1_15 mod_eg_lvl;
  Q1_15 lfo_lvl;

  /* Modulators */
  struct env amp_env;
  struct env mod_env;
};

/* API */
void voice_init(struct voice *voice, const param_value_t *restrict params);
void voice_reset(struct voice *voice);
void voice_calculate(struct voice *voice);
void voice_update(struct voice *voice);
void voice_note_on(struct voice *voice, uint8_t midi_note, uint8_t midi_velocity);
void voice_note_off(struct voice *voice);
void voice_legato(struct voice *voice, uint8_t midi_note);

#endif /* __VOICE_H__ */