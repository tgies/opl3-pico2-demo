/* Slim port of Waveshare's audio_pio.h for the RP2350-Touch-LCD-3.5 board.
 * Pin defines match the board schematic (GPIO12-17).
 *
 * The static pico_audio struct is a convenience config holder. Each .c
 * file that includes this gets its own copy initialised from the same
 * defaults; that's fine because the struct is read-only at runtime.
 */

#ifndef _PICO_AUDIO_PIO_H
#define _PICO_AUDIO_PIO_H

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#define AUDIO_PIO __CONCAT(pio, PICO_AUDIO_PIO)
#define GPIO_FUNC_PIOx __CONCAT(GPIO_FUNC_PIO, PICO_AUDIO_PIO)

/* Default to 48 kHz, MCLK = 256 * fs. ES8311 has a coefficient row at
 * (12288000, 48000). OPL3 chip rate is 49716 Hz; use OPL3_GenerateResampled
 * to bridge to 48 kHz. */
#define PICO_MCLK_FREQ      (48000 * 256)
#define PICO_SAMPLE_FREQ    48000
#define PICO_AUDIO_VOLUME   85
#define PICO_AUDIO_COUNT    1
#define PICO_AUDIO_RES_IN   16
#define PICO_AUDIO_RES_OUT  16
#define PICO_AUDIO_MIC_GAIN 3

/* Pins on Waveshare RP2350-Touch-LCD-3.5. LRCK is on the lower GPIO of the
 * LRCK/BCLK pair (LRCK=15, BCLK=16). The PIO program slaves to LRCK/BCLK
 * as inputs (codec is I2S master) so the unusual pin order doesn't matter. */
#define PICO_AUDIO_DOUT     12
#define PICO_AUDIO_DIN      13
#define PICO_AUDIO_MCLK     14
#define PICO_AUDIO_LRCLK    15
#define PICO_AUDIO_BCLK     16
#define PICO_AUDIO_PIO_1    0
#define PICO_AUDIO_PIO_2    0
#define PICO_AUDIO_SM_DOUT  0
#define PICO_AUDIO_SM_DIN   1
#define PICO_AUDIO_SM_MCLK  2

typedef struct pico_audio_struct
{
    uint32_t mclk_freq;
    uint32_t sample_freq;
    uint8_t  res_in;
    uint8_t  res_out;
    uint8_t  mic_gain;
    uint8_t  volume;
    uint8_t  channel_count;
    uint8_t  audio_dout;
    uint8_t  audio_din;
    uint8_t  audio_mclk;
    uint8_t  audio_lrclk;
    uint8_t  audio_bclk;
    PIO      pio_1;
    PIO      pio_2;
    uint8_t  sm_dout;
    uint8_t  sm_din;
    uint8_t  sm_mclk;
} pico_audio_t;

static pico_audio_t pico_audio = {
    .mclk_freq = PICO_MCLK_FREQ,
    .sample_freq = PICO_SAMPLE_FREQ,
    .res_in = PICO_AUDIO_RES_IN,
    .res_out = PICO_AUDIO_RES_OUT,
    .mic_gain = PICO_AUDIO_MIC_GAIN,
    .volume = PICO_AUDIO_VOLUME,
    .channel_count = PICO_AUDIO_COUNT,
    .audio_dout = PICO_AUDIO_DOUT,
    .audio_din = PICO_AUDIO_DIN,
    .audio_mclk = PICO_AUDIO_MCLK,
    .audio_lrclk = PICO_AUDIO_LRCLK,
    .audio_bclk = PICO_AUDIO_BCLK,
    .pio_1 = pio1,
    .pio_2 = pio2,
    .sm_dout = PICO_AUDIO_SM_DOUT,
    .sm_din = PICO_AUDIO_SM_DIN,
    .sm_mclk = PICO_AUDIO_SM_MCLK
};

#ifdef __cplusplus
extern "C" {
#endif

void dout_pio_init(void);
void din_pio_init(void);
void mclk_pio_init(void);
void set_mclk_frequency(uint32_t frequency);

/* Filled by the most recent set_mclk_frequency() call. Lets the application
 * print the achieved MCLK + ppm error later, after USB CDC has attached. */
extern uint32_t mclk_actual_hz;
extern int32_t  mclk_error_ppm;
extern uint16_t mclk_pio_div_int;
extern uint8_t  mclk_pio_div_frac;

/* Push one 16-bit stereo sample (left in low half, right in high half is
 * the convention this driver uses; see audio_pio.c). Blocks on full FIFO. */
static inline void audio_put_sample32(int32_t sample32) {
    pio_sm_put_blocking(pico_audio.pio_2, pico_audio.sm_dout, (uint32_t)sample32);
}

/* Pack 16-bit mono sample into the 32-bit word the PIO expects. */
static inline int32_t audio_pack_mono16(int16_t mono) {
    /* The Waveshare reference packs as `audio * 65536` for mono. We do the
     * same so the same PIO program clocks out 32 bits per LRCK frame, with
     * the sample in the upper 16. */
    return ((int32_t)mono) << 16;
}

#ifdef __cplusplus
}
#endif

#endif /* _PICO_AUDIO_PIO_H */
