
/*
 *  (c) Jason Wilden, 2026
 */
#include "params.h"
#include "midi.h"
#include "luts.h"

#include "drv.h"

#ifdef TRACE_ENABLED
static const char *TRACE_FILE = "params.c";
#endif

/* Sparse array for MIDI mappings */
static uint8_t param_map[128];

/*
 * Maps MIDI CC to a parameter ID.
 */
void param_init()
{
  /* Set all mappings to unassigned, this will be a sparse array */
  for (int i = 0; i < 128; i++)
  {
    param_map[i] = MIDI_CC_UNSUPPORTED;
  }

  /*
   * Now map CCs to param ids, the index is the CC number and the value is the parameter_id.
   * Any unused CCs have the MIDI_CC_UNSUPPORTED mapping.
   *
   * The free ranges are 14-31, 52-63, 102-119.  Standards values used where appropriate.
   */

  param_map[MIDI_CC_VOLUME] = AMP_LEVEL;

  param_map[MIDI_CC_ATTACKTIME] = AMP_ATTACK;
  param_map[MIDI_CC_DECAYTIME] = AMP_DECAY;
  param_map[14] = AMP_SUSTAIN;
  param_map[MIDI_CC_RELEASETIME] = AMP_RELEASE;

  param_map[15] = OSC1_WAVE;
  param_map[16] = OSC1_LEVEL;
  param_map[17] = OSC1_OCTAVE;
  param_map[18] = OSC1_SEMI;
  param_map[19] = OSC1_CENTS;
  param_map[20] = OSC1_PW;
  param_map[21] = OSC1_MOD_SOURCE;
  param_map[22] = OSC1_MOD_DEPTH;

  param_map[23] = OSC2_WAVE;
  param_map[24] = OSC2_LEVEL;
  param_map[25] = OSC2_OCTAVE;
  param_map[26] = OSC2_SEMI;
  param_map[27] = OSC2_CENTS;
  param_map[28] = OSC2_PW;
  param_map[29] = OSC2_MOD_SOURCE;
  param_map[30] = OSC2_MOD_DEPTH;

  param_map[31] = ENV1_MODE;
  param_map[52] = ENV1_ATTACK;
  param_map[53] = ENV1_DECAY;
  param_map[54] = ENV1_SUSTAIN;
  param_map[55] = ENV1_RELEASE;

  param_map[56] = LFO1_RATE;
  param_map[57] = LFO1_MODE;

  param_map[MIDI_CC_POLYMODEON] = SYNTH_VOICE_MODE;
  param_map[MIDI_CC_PORTAMENTO] = SYNTH_PORTAMENTO_ON;
  param_map[MIDI_CC_PORTAMENTOTIME] = SYNTH_PORTAMENTO_TIME;
  param_map[MIDI_CC_DETUNELEVEL] = SYNTH_UNISON_DETUNE;
}

/**
 * \brief sets a parameter
 *
 * Find the parameter to which this CC is mapped (if any) and use that to
 * find the param_id and set the value in the params array:
 *
 * 1. Check for the CC being in the param_map array and not MIDI_CC_UNSUPPORTED.
 * 2. Use the value in the CC array to1 index the params array and store the value at that
 *    array position.  The value in the CC array is the param_id value which is also the
 *    index of its value in the params array.
 *
 * Parameter values are 16-bit wide and can hold either a value or an
 * index into a LUT.
 */
enum param_mapping_type param_set_value_from_cc(uint8_t cc, uint8_t value, param_value_t params[])
{
  uint8_t param_id = param_map[cc];

  if (param_id == MIDI_CC_UNSUPPORTED)
  {
    return PARAM_UNMAPPED;
  }

  switch (param_id)
  {
  // ---------------------------------------------------
  // On/Of switches
  // -------------------------------------------------
  case AMP_NOTE_TRACK:
  case AMP_VEL_TRACK:
  case ENV1_NOTE_TRACK:
  case ENV1_VEL_TRACK:
  case FILT_KEY_TRACK:
  case SYNTH_PORTAMENTO_ON:
    params[param_id] = value > 64;
    break;

  // ---------------------------------------------------
  // Selectors
  // ---------------------------------------------------
  case AMP_MOD_SOURCE:
  case OSC1_MOD_SOURCE:
  case OSC2_MOD_SOURCE:
  case FILT_MOD_SOURCE:
    params[param_id] = param_enum_from_cc(value, MOD_SOURCE_COUNT);
    break;

  case OSC1_WAVE:
  case OSC2_WAVE:
    params[param_id] = param_enum_from_cc(value, OSC_WAVE_COUNT);
    break;

  case ENV1_MODE:
    params[param_id] = param_enum_from_cc(value, ENV_MODE_COUNT);
    break;

  case LFO1_MODE:
    params[param_id] = param_enum_from_cc(value, LFO_MODE_COUNT);
    break;

  // ---------------------------------------------------
  // Unipolar linear controls
  // ---------------------------------------------------
  case ENV1_SUSTAIN:
  case AMP_SUSTAIN:
  case FILT_RES:
  case OSC1_PW:
  case OSC2_PW:
    params[param_id] = param_cc7bit_to_uni(value);
    break;

  // ---------------------------------------------------
  // Bipolar linear controls
  // ---------------------------------------------------
  case OSC1_OCTAVE:
  case OSC2_OCTAVE:
    params[param_id] = param_cc7bit_to_bipolar(value, 2);
    break;

  case OSC1_SEMI:
  case OSC2_SEMI:
    params[param_id] = param_cc7bit_to_bipolar(value, 6);
    break;

  case OSC1_CENTS:
  case OSC2_CENTS:
    params[param_id] = param_cc7bit_to_bipolar(value, 50);
    break;

  // ---------------------------------------------------
  // Unipolar exponential controls
  // ---------------------------------------------------
  case AMP_LEVEL:
  case AMP_MOD_DEPTH:
  case OSC1_LEVEL:
  case OSC2_LEVEL:
  case OSC1_MOD_DEPTH:
  case OSC2_MOD_DEPTH:
  case LFO1_RATE:
  case GL_AMOUNT:
  case GL_TIME:
  case FILT_MOD_DEPTH:
  case SYNTH_PORTAMENTO_TIME:
  case SYNTH_UNISON_DETUNE:
    params[param_id] = midi_exp_curve_lut[value];
    break;

  // ---------------------------------------------------
  // Controls with specific LUTs used in module,
  // these only need to store the LUT index.
  // ---------------------------------------------------
  case AMP_ATTACK:
  case AMP_DECAY:
  case AMP_RELEASE:
  case ENV1_ATTACK:
  case ENV1_DECAY:
  case ENV1_RELEASE:
  case FILT_CUTOFF:
    params[param_id] = value;
    break;

  default:
  }

  // TRACE_PRINT_DEC("CC:",cc);
  // TRACE_PRINT_DEC("ID:",param_map[cc]);
  // TRACE_PRINT_DEC("VAL:",value)

  return param_id < VOICE_PARAM_COUNT ? PARAM_VOICE : PARAM_GLOBAL;
}

/*
 * Creates a default patch, this is a gated, detuned
 * sawtooth with no filtering.
 */
void param_create_default_patch(param_value_t params[])
{
  params[AMP_LEVEL] = Q15_ONE;
  params[AMP_MOD_DEPTH] = 0;
  params[AMP_MOD_SOURCE] = 0;

  params[AMP_ATTACK] = 0;
  params[AMP_DECAY] = 0;
  params[AMP_SUSTAIN] = Q15_ONE;
  params[AMP_RELEASE] = 0;
  params[AMP_NOTE_TRACK] = false;
  params[AMP_VEL_TRACK] = false;

  params[OSC1_WAVE] = OSC_SAW;
  params[OSC1_LEVEL] = Q15_HALF;
  params[OSC1_OCTAVE] = 0;
  params[OSC1_SEMI] = 0;
  params[OSC1_CENTS] = 0;
  params[OSC1_PW] = Q15_ONE;
  params[OSC1_MOD_DEPTH] = 0;
  params[OSC1_MOD_SOURCE] = 0;

  params[OSC2_WAVE] = OSC_SAW;
  params[OSC2_LEVEL] = Q15_HALF;
  params[OSC2_OCTAVE] = 1;
  params[OSC2_SEMI] = 0;
  params[OSC2_CENTS] = 10;
  params[OSC2_PW] = Q15_ONE;
  params[OSC2_MOD_DEPTH] = 0;
  params[OSC2_MOD_SOURCE] = 0;

  params[ENV1_MODE] = ENV_NORMAL;
  params[ENV1_ATTACK] = 0;
  params[ENV1_DECAY] = 0;
  params[ENV1_SUSTAIN] = Q15_ONE;
  params[ENV1_RELEASE] = 0;
  params[ENV1_NOTE_TRACK] = false;
  params[ENV1_VEL_TRACK] = false;

  params[LFO1_RATE] = 0;
  params[LFO1_MODE] = LFO_MODE_NOTE;

  params[GL_AMOUNT] = 0;
  params[GL_TIME] = 0;

  params[FILT_CUTOFF] = 127;
  params[FILT_RES] = 0;
  params[FILT_KEY_TRACK] = false;
  params[FILT_MOD_DEPTH] = 0;
  params[FILT_MOD_SOURCE] = 0;

  params[SYNTH_VOICE_MODE] = VOICE_MODE_POLY;
  params[SYNTH_PORTAMENTO_TIME] = 0;
  params[SYNTH_PORTAMENTO_ON] = false;
  params[SYNTH_UNISON_DETUNE] = 0;
}