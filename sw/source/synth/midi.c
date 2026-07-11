/*
 *  (c) Jason Wilden, 2026
 */

#include "midi.h"

#ifdef TRACE_ENABLED
#include "drv.h"
static const char *TRACE_FILE = "midi.c";
#endif

#define IS_STATUS_BYTE(__BYTE__) ((__BYTE__ >> 7) == 0x1)
#define IS_REAL_TIME(__BYTE__) ((__BYTE__ >> 3) == 0x1f)
#define IS_SINGLE_BYTE_MSG(__BYTE__) ((__BYTE__ >> 2) == 0x3D)
#define CHANNEL_MASK (0x0F)

static struct midi_msg rt_msg;
static struct midi_msg midi_msg;
static struct midi_ring_buffer midi_buffer = {{0}, 0, 0};

static struct midi_msg *midi_parse_rt_msg(uint8_t byte);
static bool midi_parse_message(struct midi_instance *midi_in, uint8_t byte);

/*
* Parses MIDI messages, this will only return a complete message (or error) and should
* be called repeatedly passing in received bytes until it does.  It manages state internally.
*/
struct midi_msg *midi_parse(struct midi_instance *midi_in, uint8_t byte)
{
  struct midi_msg *msg = NULL;

  msg = midi_parse_rt_msg(byte);

  if (msg == NULL)
  {
    if (midi_parse_message(midi_in, byte))
    {
      msg = midi_in->msg;
      midi_in->msg = NULL;
    }
  }

  return msg;
}

static void midi_reset_msg(struct midi_msg *msg)
{
  msg->len = 0;
  msg->data[0] = 0;
  msg->data[1] = 0;
  msg->data[2] = 0;
}

/*
 * This manages real-time message interleaved between the others, it does not
 * disturb partially parsed messages.
 */
static struct midi_msg *midi_parse_rt_msg(uint8_t byte)
{
  struct midi_msg *msg = NULL;

  if (IS_STATUS_BYTE(byte) && IS_REAL_TIME(byte))
  {
    rt_msg.data[0] = byte;
    rt_msg.len = 1;
    msg = &rt_msg;
  }

  return msg;
}

/*
* The main MIDI parser process.  Note that sysex messages are not supported
* but are drained, there is currently no timeout, so if an end of sysex flag 
* is not received so this will stall the MIDI parser.  
*/
static bool midi_parse_message(struct midi_instance *midi_in, uint8_t byte)
{
  struct midi_msg *msg = NULL;
  uint8_t running_status;

  if (midi_in->msg == NULL)
  {
    midi_in->msg = &midi_msg;
    midi_reset_msg(midi_in->msg);
  }

  running_status = midi_in->running_status;
  msg = midi_in->msg;

  if (midi_in->sysex_active)
  {
    if (byte == MIDI_STATUS_SYS_EX_END)
    {
      midi_in->sysex_active = false;
    }
    return false;
  }

  if (IS_STATUS_BYTE(byte))
  {
    midi_in->running_status = byte;
    midi_in->third_byte_expected = false;

    if (byte == MIDI_STATUS_SYS_EX_START)
    {
      midi_in->sysex_active = true;
      return false;
    }

    if (IS_SINGLE_BYTE_MSG(byte))
    {
      midi_in->msg->data[0] = byte;
      midi_in->msg->len = 1;
      return true;
    }

    return false;
  }

  if (midi_in->channel != MIDI_OMNI && (running_status & CHANNEL_MASK) != (midi_in->channel - 1))
  {
    return false;
  }

  if (midi_in->third_byte_expected)
  {
    midi_in->third_byte_expected = false;
    msg->data[2] = byte;
    msg->len = 3;
    return true;
  }

  if (running_status == MIDI_STATUS_INVALID)
  {
    return false;
  }

  uint8_t switch_status = (running_status < 0xF0)
                              ? (running_status & ~CHANNEL_MASK)
                              : running_status;

  switch (switch_status)
  {
  case MIDI_STATUS_NOTE_ON:
  case MIDI_STATUS_NOTE_OFF:
  case MIDI_STATUS_CONTROL_CHANGE:
  case MIDI_STATUS_PITCH_BEND:
  case MIDI_STATUS_POLY_PRESSURE:
    midi_in->third_byte_expected = true;
    msg->data[0] = running_status;
    msg->data[1] = byte;
    msg->len = 2;
    return false;

  case MIDI_STATUS_PROGRAM_CHANGE:
  case MIDI_STATUS_CHANNEL_PRESSURE:
    midi_in->third_byte_expected = false;
    msg->data[0] = running_status;
    msg->data[1] = byte;
    msg->len = 2;
    return true;

  case MIDI_STATUS_SONG_POS:
    midi_in->third_byte_expected = true;
    msg->data[0] = running_status;
    msg->data[1] = byte;
    msg->len = 2;
    midi_in->running_status = MIDI_STATUS_INVALID;
    return false;

  case MIDI_STATUS_SONG_SELECT:
  case MIDI_STATUS_TIME_CODE:
    midi_in->third_byte_expected = true;
    msg->data[0] = running_status;
    msg->data[1] = byte;
    msg->len = 2;
    midi_in->running_status = MIDI_STATUS_INVALID;
    return true;
  }

  midi_in->running_status = MIDI_STATUS_INVALID;
  return false;
}

/*
 * Write a byte to the MIDI buffer
 *
 * This function is called by the MIDI ISR to write incoming MIDI bytes to the buffer.
 */
void midi_buffer_write(uint8_t byte)
{
  uint16_t nextHead = (midi_buffer.head + 1) % MIDI_BUFFER_SIZE;

  if (nextHead != midi_buffer.tail)
  {
    midi_buffer.buffer[midi_buffer.head] = byte;
    midi_buffer.head = nextHead;
  }
}

/*
 * Read a byte from the MIDI buffer
 */
bool midi_buffer_read(uint8_t *data)
{
  if (midi_buffer.head != midi_buffer.tail)
  {
    *data = midi_buffer.buffer[midi_buffer.tail];
    midi_buffer.tail = (midi_buffer.tail + 1) % MIDI_BUFFER_SIZE;
    return true;
  }
  return false;
}