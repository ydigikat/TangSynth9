/*
 * (c) Jason Wilden, 2026
 */

#include "unity.h"
#include "midi.h"

static struct midi_instance inst;

/*
 * Feeds a stream of bytes to the midi parser returning the last
 * msg parsed or NULL if no messages found.
 */
static struct midi_msg *feed(const uint8_t *bytes, size_t n)
{
  struct midi_msg *msg = NULL;
  for (size_t i = 0; i < n; i++)
  {
    struct midi_msg *m = midi_parse(&inst, bytes[i]);
    if (m)
      msg = m;
  }

  return msg;
}

#define FEED(...) ({uint8_t _bytes[] = {__VA_ARGS__}; feed(_bytes,sizeof(_bytes)); })

/* -------------------------------------------------------------------------
 * Fixture setup/teardown
 * ---------------------------------------------------------------------- */

void setUp(void)
{
  inst = (struct midi_instance){.channel = MIDI_OMNI};
}

void tearDown(void) {}

/* -------------------------------------------------------------------------
 * 3-byte channel tests
 * ---------------------------------------------------------------------- */

void note_on_returns_msg_at_third_byte(void)
{
  TEST_ASSERT_NULL(midi_parse(&inst, 0x90));      /* Status */
  TEST_ASSERT_NULL(midi_parse(&inst, 0x3C));      /* note */
  struct midi_msg *msg = midi_parse(&inst, 0x7F); /* velocity */
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_NOTE_ON, msg->data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x3C, msg->data[1]);
  TEST_ASSERT_EQUAL_UINT8(0x7F, msg->data[2]);
  TEST_ASSERT_EQUAL_size_t(3, msg->len);
}

void note_off_parsed_correctly(void)
{
  struct midi_msg *msg = FEED(0x80, 0x3C, 0x40);
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_NOTE_OFF, msg->data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x3C, msg->data[1]);
  TEST_ASSERT_EQUAL_UINT8(0x40, msg->data[2]);
}

void control_change_parsed_correctly(void)
{
  struct midi_msg *msg = FEED(0xB0, MIDI_CC_VOLUME, 100);
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_CONTROL_CHANGE, msg->data[0]);
  TEST_ASSERT_EQUAL_UINT8(MIDI_CC_VOLUME, msg->data[1]);
  TEST_ASSERT_EQUAL_UINT8(100, msg->data[2]);
}

void pitch_bend_parsed_correctly(void)
{
  struct midi_msg *msg = FEED(0xE0, 0x00, 0x40); /* centre position */
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_PITCH_BEND, msg->data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x00, msg->data[1]);
  TEST_ASSERT_EQUAL_UINT8(0x40, msg->data[2]);
  TEST_ASSERT_EQUAL_size_t(3, msg->len);
}

void poly_pressure_parsed_correctly(void)
{
  struct midi_msg *msg = FEED(0xA0, 0x3C, 0x60);
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_POLY_PRESSURE, msg->data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x3C, msg->data[1]);
  TEST_ASSERT_EQUAL_UINT8(0x60, msg->data[2]);
}

/* -------------------------------------------------------------------------
 * 2-byte channel tests
 * ---------------------------------------------------------------------- */

void program_change_returns_on_second_byte(void)
{
  TEST_ASSERT_NULL(midi_parse(&inst, 0xC0));      /* status — no msg yet   */
  struct midi_msg *msg = midi_parse(&inst, 0x05); /* program — complete    */
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_PROGRAM_CHANGE, msg->data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x05, msg->data[1]);
  TEST_ASSERT_EQUAL_size_t(2, msg->len);
}

void channel_pressure_returns_on_second_byte(void)
{
  struct midi_msg *msg = FEED(0xD0, 0x7F);
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_CHANNEL_PRESSURE, msg->data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x7F, msg->data[1]);
  TEST_ASSERT_EQUAL_size_t(2, msg->len);
}

/* -------------------------------------------------------------------------
 * Running status tests
 * ---------------------------------------------------------------------- */
void running_status_second_note_on(void)
{
  FEED(0x90, 0x3C, 0x7F);                  /* first note — consume  */
  struct midi_msg *msg = FEED(0x48, 0x64); /* second note, no status*/
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_NOTE_ON, msg->data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x48, msg->data[1]); /* E4                    */
  TEST_ASSERT_EQUAL_UINT8(0x64, msg->data[2]); /* velocity 100          */
}

void running_status_multiple_cc(void)
{
  FEED(0xB0, MIDI_CC_VOLUME, 100);
  struct midi_msg *msg = FEED(MIDI_CC_PAN, 64);
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_CONTROL_CHANGE, msg->data[0]);
  TEST_ASSERT_EQUAL_UINT8(MIDI_CC_PAN, msg->data[1]);
  TEST_ASSERT_EQUAL_UINT8(64, msg->data[2]);
}

void running_status_cleared_by_new_status(void)
{
  FEED(0x90, 0x3C, 0x7F);                    /* Note On running status */
  FEED(0x80);                                /* New status: Note Off  */
  TEST_ASSERT_NULL(midi_parse(&inst, 0x48)); /* first data byte only  */
}

/* -------------------------------------------------------------------------
 * Intermidate state tests
 * ---------------------------------------------------------------------- */
void no_message_after_status_only(void)
{
  TEST_ASSERT_NULL(midi_parse(&inst, 0x90));
}

void no_message_after_first_data_byte(void)
{
  midi_parse(&inst, 0x90);
  TEST_ASSERT_NULL(midi_parse(&inst, 0x3C));
}

/* -------------------------------------------------------------------------
 * Sysex draining
 * ---------------------------------------------------------------------- */
void sysex_bytes_are_swallowed(void)
{
  /* SysEx start + arbitrary manufacturer data — nothing returned */
  TEST_ASSERT_NULL(midi_parse(&inst, MIDI_STATUS_SYS_EX_START));
  TEST_ASSERT_NULL(midi_parse(&inst, 0x41)); /* Roland ID               */
  TEST_ASSERT_NULL(midi_parse(&inst, 0x10));
  TEST_ASSERT_NULL(midi_parse(&inst, 0x42));
  TEST_ASSERT_NULL(midi_parse(&inst, MIDI_STATUS_SYS_EX_END));
}

void message_received_after_sysex_ends(void)
{
  FEED(MIDI_STATUS_SYS_EX_START, 0x41, 0x10, MIDI_STATUS_SYS_EX_END);
  struct midi_msg *msg = FEED(0x90, 0x3C, 0x7F);
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_NOTE_ON, msg->data[0]);
}

void note_on_during_sysex_is_ignored(void)
{
  /* A Note On status arriving mid-SysEx should not produce a message.
     The SysEx active flag should suppress it. */
  midi_parse(&inst, MIDI_STATUS_SYS_EX_START);
  TEST_ASSERT_NULL(midi_parse(&inst, 0x90)); /* looks like Note On status */
  TEST_ASSERT_NULL(midi_parse(&inst, 0x3C));
  TEST_ASSERT_NULL(midi_parse(&inst, 0x7F));
}

/* -------------------------------------------------------------------------
 * Realtime Messages
 * ---------------------------------------------------------------------- */
void clock_returned_mid_message(void)
{
  midi_parse(&inst, 0x90);
  midi_parse(&inst, 0x3C);
  /* Clock arrives before velocity byte */
  struct midi_msg *rt = midi_parse(&inst, MIDI_STATUS_CLOCK);
  TEST_ASSERT_NOT_NULL(rt);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_CLOCK, rt->data[0]);
  TEST_ASSERT_EQUAL_size_t(1, rt->len);
}

void note_completes_after_interleaved_clock(void)
{
  midi_parse(&inst, 0x90);
  midi_parse(&inst, 0x3C);
  midi_parse(&inst, MIDI_STATUS_CLOCK);           /* interleaved RT        */
  struct midi_msg *msg = midi_parse(&inst, 0x7F); /* velocity — completes  */
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_NOTE_ON, msg->data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x7F, msg->data[2]);
}

void active_sense_is_single_byte(void)
{
  struct midi_msg *msg = midi_parse(&inst, MIDI_STATUS_ACTIVE_SENSE);
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_ACTIVE_SENSE, msg->data[0]);
  TEST_ASSERT_EQUAL_size_t(1, msg->len);
}

void start_stop_continue_are_single_byte(void)
{
  struct midi_msg *msg;

  msg = midi_parse(&inst, MIDI_STATUS_START);
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_START, msg->data[0]);

  msg = midi_parse(&inst, MIDI_STATUS_STOP);
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_STOP, msg->data[0]);

  msg = midi_parse(&inst, MIDI_STATUS_CONTINUE);
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_CONTINUE, msg->data[0]);
}

/* -------------------------------------------------------------------------
 * System tests
 * ---------------------------------------------------------------------- */
void song_position_pointer(void)
{
  /* Song Pos is 3 bytes but clears running status afterwards */
  struct midi_msg *msg = FEED(MIDI_STATUS_SONG_POS, 0x00, 0x00);
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_SONG_POS, msg->data[0]);
  TEST_ASSERT_EQUAL_size_t(3, msg->len);
}

void song_select(void)
{
  struct midi_msg *msg = FEED(MIDI_STATUS_SONG_SELECT, 0x03);
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_SONG_SELECT, msg->data[0]);
  TEST_ASSERT_EQUAL_UINT8(0x03, msg->data[1]);
}

/* -------------------------------------------------------------------------
 * Channel filtering
 * ---------------------------------------------------------------------- */
void channel_filter_accepts_matching_channel(void)
{
  inst.channel = 1;                              /* listen on ch 1*/
  struct midi_msg *msg = FEED(0x90, 0x3C, 0x7F); /* ch 1 Note On*/
  TEST_ASSERT_NOT_NULL(msg);
}


void channel_filter_rejects_other_channel(void)
{
  inst.channel = 1;                               /* listen on ch 1        */
  struct midi_msg *msg = FEED(0x92, 0x3C, 0x7F); /* ch 3 Note On          */
  TEST_ASSERT_NULL(msg);
}

void omni_accepts_ch2(void)
{
  inst.channel = MIDI_OMNI;
  struct midi_msg *msg = FEED(0x91, 0x3C, 0x7F); /* ch 2 Note On          */
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_NOTE_ON, msg->data[0] & 0xF0);
}

void omni_accepts_ch6(void)
{
  inst.channel = MIDI_OMNI;
  struct midi_msg *msg = FEED(0x95, 0x48, 0x40); /* ch 6 Note On          */
  TEST_ASSERT_NOT_NULL(msg);
  TEST_ASSERT_EQUAL_UINT8(MIDI_STATUS_NOTE_ON, msg->data[0] & 0xF0);
}

void channel_filter_accepts_ch1(void)
{
  inst.channel = 1;
  struct midi_msg *msg = FEED(0x90, 0x3C, 0x7F); /* ch 1 Note On, status = 0x90 */
  TEST_ASSERT_NOT_NULL(msg);
}

void channel_filter_accepts_ch16(void)
{
  inst.channel = 16;
  struct midi_msg *msg = FEED(0x9F, 0x3C, 0x7F); /* ch 16 Note On, status = 0x9F */
  TEST_ASSERT_NOT_NULL(msg);
}

/* -------------------------------------------------------------------------
 * Buffer
 * ---------------------------------------------------------------------- */
void can_buffer_byte(void)
{
  uint8_t byte = 0x9F;
  uint8_t byte_out = 0x00;

  midi_buffer_write(byte);
  midi_buffer_read(&byte_out);

  TEST_ASSERT_EQUAL_UINT8(byte, byte_out);
}

void buffer_discards_incoming_when_full(void)
{
  uint8_t byte_out = 0x00;

  /* Fill buffer */
  midi_buffer_write(0x01);
  midi_buffer_write(0x02);
  midi_buffer_write(0x03);
  midi_buffer_write(0x04);
  midi_buffer_write(0x05);
  midi_buffer_write(0x06);
  midi_buffer_write(0x07);
  midi_buffer_write(0x08);
  midi_buffer_write(0x09);
  midi_buffer_write(0x0A);
  midi_buffer_write(0x0B);
  midi_buffer_write(0x0C);
  midi_buffer_write(0x0D);
  midi_buffer_write(0x0E);
  midi_buffer_write(0x0F);  

  /* Add one more */
  midi_buffer_write(0x10);

  /* Should not be stored into buffer */
  midi_buffer_read(&byte_out);
  TEST_ASSERT_EQUAL_UINT8(0x01,byte_out);
}


/* -------------------------------------------------------------------------
 * Runner
 * ---------------------------------------------------------------------- */
int main(void)
{
  UNITY_BEGIN();
  /* 3-byte channel messages*/
  RUN_TEST(note_on_returns_msg_at_third_byte);
  RUN_TEST(note_off_parsed_correctly);
  RUN_TEST(control_change_parsed_correctly);
  /* 2-byte channel messages*/
  RUN_TEST(pitch_bend_parsed_correctly);
  RUN_TEST(poly_pressure_parsed_correctly);
  RUN_TEST(program_change_returns_on_second_byte);
  RUN_TEST(channel_pressure_returns_on_second_byte);
  /* Running status */
  RUN_TEST(running_status_cleared_by_new_status);
  RUN_TEST(running_status_second_note_on);
  RUN_TEST(running_status_multiple_cc);
  /* Intermediate states*/
  RUN_TEST(no_message_after_first_data_byte);
  RUN_TEST(no_message_after_status_only);
  /* Sysex draining */
  RUN_TEST(sysex_bytes_are_swallowed);
  RUN_TEST(note_on_during_sysex_is_ignored);
  RUN_TEST(message_received_after_sysex_ends);
  /* Reatime*/
  RUN_TEST(clock_returned_mid_message);
  RUN_TEST(note_completes_after_interleaved_clock);
  RUN_TEST(active_sense_is_single_byte);
  RUN_TEST(start_stop_continue_are_single_byte);
  /* System */
  RUN_TEST(song_position_pointer);
  RUN_TEST(song_select);
  /* Channel filter*/  
  RUN_TEST(channel_filter_accepts_matching_channel);
  RUN_TEST(channel_filter_rejects_other_channel);
  RUN_TEST(channel_filter_accepts_ch16);
  RUN_TEST(channel_filter_accepts_ch1);
  RUN_TEST(omni_accepts_ch2);
  RUN_TEST(omni_accepts_ch6);
  /* Buffer*/
  RUN_TEST(can_buffer_byte);
  RUN_TEST(buffer_discards_incoming_when_full);
  
  return UNITY_END();
}