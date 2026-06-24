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

/*
* The synth is divided between the SOC and the audio pipeline. The SOC is responsible
* for control plane functions,MIDI, patch management, modulation and voice lifecycles.  
* It does not generate audio instead rendering the data required by the audio pipeline 
* to do this.  This is passed to the pipeline via the voice ram (VRAM).
*
* The synth renders data at a slower rate than samples are generated, it counts the
* number of audio interrupts (sample rate) from the audio pipeline and updates VRAM 
* every time the count reaches the block size.
*/
struct synth
{
  uint8_t midi_channel;

  struct voice voice[MAX_VOICES];
  param_value_t  params[VOICE_PARAM_COUNT];
};

/* API */
void synth_init(struct synth *synth);
void synth_handle_midi(struct synth *synth, struct midi_msg *msg);
void synth_render(struct synth *synth);
uint8_t synth_execute(struct synth *synth, uint8_t sample_count);

#endif /* __CONTROLLER_H__*/