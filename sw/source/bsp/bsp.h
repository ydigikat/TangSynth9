/*
 *  (c) Jason Wilden, 2026
 */
#ifndef __BSP_H__
#define __BSP_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/* ----------------------------------------------------------------------------
 * Configuration
 * --------------------------------------------------------------------------*/
#define SYS_FREQ (24000000UL)
#define SRAM_BASE (0x00000000UL)
#define PERIPH_BASE (0x80000000UL)

/* ----------------------------------------------------------------------------
 * Bit Helpers
 * --------------------------------------------------------------------------*/
#define SET_BIT(REG, BIT) ((REG) |= (BIT))
#define CLEAR_BIT(REG, BIT) ((REG) &= ~(BIT))
#define READ_BIT(REG, BIT) ((REG) & (BIT))
#define CLEAR_REG(REG) ((REG) = (0x0))
#define WRITE_REG(REG, VAL) ((REG) = (VAL))
#define READ_REG(REG) ((REG))
#define MODIFY_REG(REG, CLEARMASK, SETMASK) WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))
#define _VAL2FLD(field, value) (((uint32_t)(value) << field##_Pos) & field##_Msk)
#define _FLD2VAL(field, value) (((uint32_t)(value) & field##_Msk) >> field##_Pos)

/* ----------------------------------------------------------------------------
 * Register data structures
 * --------------------------------------------------------------------------*/
#define __I volatile const
#define __O volatile
#define __IO volatile

typedef struct
{
  __IO uint32_t BSR; /* Bit set/reset register  */
} GPO_t;


typedef struct 
{
    __IO uint32_t CR; /* Control register */
    __I uint32_t SR;  /* Status register  */
    __O uint32_t TD;  /* Trace data register */    
} TRACE_t;

typedef struct 
{
    __IO uint32_t CR; /* Control register */
} APCR_t;

typedef struct
{
  __IO uint32_t CR;   /* Control register  : [10:0] baud divisor            */
  __I  uint32_t SR;   /* Status register   : [0] rx_valid, [1] framing err  */
  __I  uint32_t RD;   /* RX data register  : [7:0] received byte            */
} MIDI_t;

/* ----------------------------------------------------------------------------
 * Memory Map
 * --------------------------------------------------------------------------*/
#define GPO1_BASE (PERIPH_BASE + 0x00UL)
#define TRACE_BASE (PERIPH_BASE + 0x40UL)
#define APCR_BASE (PERIPH_BASE + 0x80UL)
#define MIDI_BASE  (PERIPH_BASE + 0xC0UL)

/* ----------------------------------------------------------------------------
 * Built-in peripheral instances (MMIO)
 * --------------------------------------------------------------------------*/
#define GPO1 ((GPO_t *)GPO1_BASE)
#define TRACE ((TRACE_t *)TRACE_BASE)
#define APCR ((APCR_t *)APCR_BASE)
#define MIDI  ((MIDI_t *)MIDI_BASE)


/* ----------------------------------------------------------------------------
 * GPO Registers
 * --------------------------------------------------------------------------*/

 /* Bit set/reset register */
#define GPO_BSR_0_Pos (0U)
#define GPO_BSR_0_Msk (0x1UL << GPO_BSR_0_Pos)
#define GPO_BSR_0 GPO_BSR_0_Msk
#define GPO_BSR_1_Pos (1U)
#define GPO_BSR_1_Msk (0x1UL << GPO_BSR_1_Pos)
#define GPO_BSR_1 GPO_BSR_1_Msk
#define GPO_BSR_2_Pos (2U)
#define GPO_BSR_2_Msk (0x1UL << GPO_BSR_2_Pos)
#define GPO_BSR_2 GPO_BSR_2_Msk
#define GPO_BSR_3_Pos (3U)
#define GPO_BSR_3_Msk (0x1UL << GPO_BSR_3_Pos)
#define GPO_BSR_3 GPO_BSR_3_Msk
#define GPO_BSR_4_Pos (4U)
#define GPO_BSR_4_Msk (0x1UL << GPO_BSR_4_Pos)
#define GPO_BSR_4 GPO_BSR_4_Msk
#define GPO_BSR_5_Pos (5U)
#define GPO_BSR_5_Msk (0x1UL << GPO_BSR_5_Pos) >
#define GPO_BSR_5 GPO_BSR_5_Msk
#define GPO_BSR_6_Pos (6U)
#define GPO_BSR_6_Msk (0x1UL << GPO_BSR_6_Pos)
#define GPO_BSR_6 GPO_BSR_6_Msk
#define GPO_BSR_7_Pos (7U)
#define GPO_BSR_7_Msk (0x1UL << GPO_BSR_7_Pos)
#define GPO_BSR_7 GPO_BSR_7_Msk
#define GPO_BSR_8_Pos (8U)
#define GPO_BSR_8_Msk (0x1UL << GPO_BSR_8_Pos)
#define GPO_BSR_8 GPO_BSR_8_Msk
#define GPO_BSR_9_Pos (9U)
#define GPO_BSR_9_Msk (0x1UL << GPO_BSR_9_Pos)
#define GPO_BSR_9 GPO_BSR_9_Msk
#define GPO_BSR_10_Pos (10U)
#define GPO_BSR_10_Msk (0x1UL << GPO_BSR_10_Pos)
#define GPO_BSR_10 GPO_BSR_10_Msk
#define GPO_BSR_11_Pos (11U)
#define GPO_BSR_11_Msk (0x1UL << GPO_BSR_11_Pos)
#define GPO_BSR_11 GPO_BSR_11_Msk
#define GPO_BSR_12_Pos (12U)
#define GPO_BSR_12_Msk (0x1UL << GPO_BSR_12_Pos)
#define GPO_BSR_12 GPO_BSR_12_Msk
#define GPO_BSR_13_Pos (13U)
#define GPO_BSR_13_Msk (0x1UL << GPO_BSR_13_Pos)
#define GPO_BSR_13 GPO_BSR_13_Msk
#define GPO_BSR_14_Pos (14U)
#define GPO_BSR_14_Msk (0x1UL << GPO_BSR_14_Pos)
#define GPO_BSR_14 GPO_BSR_14_Msk
#define GPO_BSR_15_Pos (15U)
#define GPO_BSR_15_Msk (0x1UL << GPO_BSR_15_Pos)
#define GPO_BSR_15 GPO_BSR_15_Msk

/* ----------------------------------------------------------------------------
 * Trace Registers
 * --------------------------------------------------------------------------*/

/* Control register */
#define TRACE_CR_DIV_Pos (0U)
#define TRACE_CR_DIV_Msk (0x7FFUL << TRACE_CR_DIV_Pos) /* 0x000007FF */
#define TRACE_CR_DIV (TRACE_CR_DIV_Msk)                /* [10:0] divisor */

/* Status register */
#define TRACE_SR_TXRDY_Pos (0U)
#define TRACE_SR_TXRDY_Msk (0x1UL << TRACE_SR_TXRDY_Pos) /* 0x00000001 */
#define TRACE_SR_TXRDY (TRACE_SR_TXRDY_Msk)              /* [0] Ready for TX */
#define TRACE_SR_RXRDY_Pos (1U)
#define TRACE_SR_RXRDY_Msk (0x1UL << TRACE_SR_RXRDY_Pos) /* 0x00000002 */
#define TRACE_SR_RXRDY (TRACE_SR_RXRDY_Msk)              /* [1] Ready for RX */

/* Data register, serial data out */
#define TRACE_TX_DAT_Pos (0U)
#define TRACE_TX_DAT_Msk (0xFFUL << TRACE_TX_DAT_Pos)  /* 0x000000FF*/
#define TRACE_TX_DAT (TRACE_TX_DAT_Msk)                /* [7:0] output data byte */

#define TRACE_SUCCESS (0UL)           /*!< Successful call */
#define TRACE_TIMEOUT (2UL)           /*!< Call timed out */
#define TRACE_TIMEOUT_COUNT (10000)   /*!< Timeout counter value */

static const uint8_t TRACE_DIV = (SYS_FREQ/(115200*16)-1); /*!< 115200 divisor */

/* ----------------------------------------------------------------------------
 * Audio Pipeline Control Register (APCR)
 * --------------------------------------------------------------------------*/
#define APCR_CR_VRAM_UPDATE_Pos (0U)
#define APCR_CR_VRAM_UPDATE_Msk (0x1UL << APCR_CR_VRAM_UPDATE_Pos)
#define APCR_CR_VRAM_UPDATE (APCR_CR_VRAM_UPDATE_Msk)   // [0] Update from VRAM


/* ----------------------------------------------------------------------------
 * MIDI Registers
 * --------------------------------------------------------------------------*/

/* Control register */
#define MIDI_CR_DIV_Pos     (0U)
#define MIDI_CR_DIV_Msk     (0x7FFUL << MIDI_CR_DIV_Pos)   /* 0x000007FF */
#define MIDI_CR_DIV         (MIDI_CR_DIV_Msk)               /* [10:0] baud divisor */

/* Status register */
#define MIDI_SR_RXRDY_Pos   (0U)
#define MIDI_SR_RXRDY_Msk   (0x1UL << MIDI_SR_RXRDY_Pos)   /* 0x00000001 */
#define MIDI_SR_RXRDY       (MIDI_SR_RXRDY_Msk)             /* [0] byte received */

#define MIDI_SR_ERR_Pos     (1U)
#define MIDI_SR_ERR_Msk     (0x1UL << MIDI_SR_ERR_Pos)     /* 0x00000002 */
#define MIDI_SR_ERR         (MIDI_SR_ERR_Msk)               /* [1] framing error */

/* RX data register */
#define MIDI_RD_DAT_Pos     (0U)
#define MIDI_RD_DAT_Msk     (0xFFUL << MIDI_RD_DAT_Pos)    /* 0x000000FF */
#define MIDI_RD_DAT         (MIDI_RD_DAT_Msk)               /* [7:0] received byte */

/* MIDI baud: 31250 bps, 16x oversampling.  div = (SYS_FREQ / (31250 * 16)) - 1 */
static const uint16_t MIDI_DIV = (SYS_FREQ / (31250U * 16U) - 1U);  /* = 47 @ 24 MHz */

#endif /* __BSP_H__ */