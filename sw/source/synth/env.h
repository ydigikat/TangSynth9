/*
 *  (c) Jason Wilden, 2026
 */

#ifndef __ENV_H__
#define __ENV_H__

#include <stdint.h>
#include <stdbool.h>

#include "luts.h"

/*
* This is a fixed point port of my Frugi floating point generator which uses Nigel
* Redmon's algorithm as discussed in Will Pirkle's book.  This is block based
* rather than the original's per sample calculation.
*
* The picorv32 soft-core has no FPU and no support for exp/log() so the asymtoptic segment 
* coefficients are precomputed for all 128 MIDI control values by a Python script
* and stored in lookup tables.  The max segment time is 8s.
*
* There is a single software division left in the RTZ path which is only called once
* during a voice steal event so the divide is an acceptable cost.
*/
enum env_state
{
  ENV_OFF,
  ENV_ATTACK,
  ENV_DECAY,
  ENV_SUSTAIN,
  ENV_RELEASE,
  ENV_SHUTDOWN,
  ENV_MAX
};

struct env
{
  /* Params - table index */
  uint8_t attack;          /* 0-127 */
  uint8_t decay;           /* 0-127 */  
  uint8_t release;         /* 0-127 */

  SQ1_15   sustain;         /* Q1.15 normalised value */
  
  uint8_t mode;            /* enum env_mode */
  bool    note_track;     
  bool    velocity_track;

  /* State */
  enum env_state state;
  SQ1_15   level;            /* current envelope output */

  /* Coefficients looked up at update time (not per-block) */
  Q1_15   attack_coeff;
  Q1_15   decay_coeff;
  Q1_15   release_coeff;

  /* 
   * Attack_overshoot and Release_overshoot are sustain-independent and come straight out 
   * of luts (both signed).   
   * 
   * The Decay_overshoot depends on `sustain`, so it's uses the env_decay_overshoot_base_lut[decay] 
   * plus one Q15 multiply, done once when parms are updated. 
  */
  SQ1_15 attack_overshoot;   /* always >= 0 */
  SQ1_15 decay_overshoot;    /* negative (falling toward sustain) */
  SQ1_15 release_overshoot;  /* always <= 0 */

  /* RTZ / shutdown ramp */
  SQ1_15 inc_shutdown;       /* signed Q1.15 per-block decrement */
};

/* API */
void env_init(struct env *env);
void env_reset(struct env *env);
void env_render(struct env *env, SQ1_15 *output);
void env_note_on(struct env *env, uint8_t midi_note, uint8_t midi_velocity);
void env_note_off(struct env *env);
void env_rtz(struct env *env);
void env_update(struct env *env, uint8_t attack, uint8_t decay, SQ1_15 sustain, uint8_t release,
                uint8_t mode, bool note_track, bool velocity_track);



#endif /* __ENV_H__ */