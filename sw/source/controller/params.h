/*
 *  (c) Jason Wilden, 2026
 */
#ifndef __PARAMS_H__
#define __PARAMS_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "midi.h"
#include "types.h"

/*
* A param_value_t is a 16-bit value which can contain a:
*   Normalised value -1.0:1.0 (approx) [Q1.15]
*   Index into an enum [uint8_t]
*   Index into a LUT [uint8_t]
*   An bipolar offset value [int8_t]
*   A flag [bool]
*
* See: param_set_value_from_cc()
*/
typedef uint16_t param_value_t;

enum param_mapping_type
{
  PARAM_UNMAPPED,
  PARAM_VOICE,
  PARAM_GLOBAL
};

/* Voice parameters */
enum param_id
{                   
  AMP_LEVEL,     
  AMP_MOD_SOURCE,
  AMP_MOD_DEPTH,

  AMP_ATTACK,
  AMP_DECAY,
  AMP_SUSTAIN,
  AMP_RELEASE,
  AMP_NOTE_TRACK,
  AMP_VEL_TRACK,

  OSC1_WAVE,
  OSC1_LEVEL,
  OSC1_OCTAVE,
  OSC1_SEMI,
  OSC1_CENTS,
  OSC1_PW,
  OSC1_MOD_SOURCE,
  OSC1_MOD_DEPTH,

  OSC2_WAVE,
  OSC2_LEVEL,
  OSC2_OCTAVE,
  OSC2_SEMI,
  OSC2_CENTS,
  OSC2_PW,
  OSC2_MOD_SOURCE,
  OSC2_MOD_DEPTH,

  ENV1_MODE,
  ENV1_ATTACK,
  ENV1_DECAY,
  ENV1_SUSTAIN,
  ENV1_RELEASE,
  ENV1_NOTE_TRACK,
  ENV1_VEL_TRACK,

  LFO1_RATE,
  LFO1_MODE,

  GL_AMOUNT,
  GL_TIME,

  FILT_CUTOFF,
  FILT_RES,
  FILT_KEY_TRACK,
  FILT_MOD_SOURCE,
  FILT_MOD_DEPTH,

  /* End of voice params marker */
  VOICE_PARAM_COUNT,

  /* Global parameters */
  SYNTH_VOICE_MODE,
  SYNTH_PORTAMENTO_ON,
  SYNTH_PORTAMENTO_TIME,
  SYNTH_UNISON_DETUNE,

  PARAM_COUNT
};

enum osc_wave
{
  OSC_TRIANGLE,
  OSC_SAW,
  OSC_PULSE,
  OSC_WAVE_COUNT
};

enum env_mode
{
  ENV_NORMAL,
  ENV_BIASED,
  ENV_INVERTED,
  ENV_INVERTED_BIASED,
  ENV_MODE_COUNT
};

enum mod_source
{
  LFO1_TRI,
  LFO1_SAW,
  LFO1_RAMP,
  LFO1_SQUARE,
  LFO1_SANDH,  
  ENV1_LEVEL,  
  MOD_SOURCE_COUNT
};

enum lfo_mode
{
  LFO_MODE_FREE,
  LFO_MODE_NOTE,

  LFO_MODE_COUNT
};

enum voice_mode
{
  VOICE_MODE_POLY,
  VOICE_MODE_MONO,
  VOICE_MODE_COUNT
};

/* Conversion helpers */
static inline param_value_t param_enum_from_cc(uint8_t value, uint8_t count)
{
    param_value_t e = (uint16_t)(value * count / 128);
    return e < count ? e : count - 1;  /* clamp */
}

static inline param_value_t param_cc7bit_to_uni(uint8_t value)
{
  return (param_value_t)((uint16_t)value * Q15_ONE / 127);
}

static inline param_value_t param_cc7bit_to_bipolar(uint8_t value, int8_t range)
{
  return (param_value_t)(int8_t)(((int16_t)value - 64) * range / 64);
}



/* API */
void param_init();
enum param_mapping_type param_set_value_from_cc(uint8_t cc, uint8_t value, param_value_t params[]);
void param_create_default_patch(param_value_t params[]);


#endif /* __PARAMS_H__ */