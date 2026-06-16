/*
*  (c) Jason Wilden, 2026
*/
#ifndef __SYNTH_H__
#define __SYNTH_H__

#include <stdint.h>
#include <stddef.h>

#include "types.h"
#include "midi.h"
#include "voice.h"
#include "params.h"

#define MAX_VOICES (8)
#define BLOCK_SIZE (32)

struct synth
{
  uint8_t midi_channel;

  struct voice voice[MAX_VOICES];
  uint8_t params[VOICE_PARAM_COUNT];
};

/* API */
void synth_init(struct synth *synth);
void synth_handle_midi(struct synth *synth, struct midi_msg *msg);
void synth_calculate(struct synth *synth);

#endif /* __SYNTH_H__*/