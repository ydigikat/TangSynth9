/*
 *  (c) Jason Wilden, 2026
 */

#ifndef __VOICE_H__
#define __VOICE_H__

#include <stdint.h>
#include "types.h"
#include "params.h"
#include "env.h"
#include "lfo.h"

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

  /* Modulator values */
  SQ1_15 mod_value[MOD_SOURCE_COUNT];

  /* Parameters */
  const param_value_t *params;

  /* FCW 24-bit (pitch)*/
  Q24_0 fcw, steal_fcw; 

  /* Modulators */
  struct env amp_env;
  struct env mod_env;
  struct lfo lfo1;
};

/* API */
void voice_init(struct voice *voice, const param_value_t *restrict params);
void voice_reset(struct voice *voice);
void voice_render(struct voice *voice);
void voice_update(struct voice *voice);
void voice_note_on(struct voice *voice, uint8_t midi_note, uint8_t midi_velocity);
void voice_note_off(struct voice *voice);
void voice_legato(struct voice *voice, uint8_t midi_note);

#endif /* __VOICE_H__ */