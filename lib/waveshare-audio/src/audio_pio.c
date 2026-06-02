/* Slim port of Waveshare's audio_pio.c. Demo functions (Music_out,
 * Sine_440hz_out, Loopback_test, Happy_birthday_out) removed; the
 * three init functions and the MCLK-freq setter remain.
 *
 * Copyright (C) Waveshare. MIT-style license per the upstream file header.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "audio_pio.h"
#include "audio_pio.pio.h"

uint32_t mclk_actual_hz   = 0;
int32_t  mclk_error_ppm   = 0;
uint16_t mclk_pio_div_int = 0;
uint8_t  mclk_pio_div_frac = 0;

void set_mclk_frequency(uint32_t mclk_freq)
{
    uint32_t sys_hz = clock_get_hz(clk_sys);
    /* MCLK PIO loops 5 instructions/period, so PIO clk = 5*MCLK. div is 16.8
     * fixed point, rounded to nearest. */
    uint64_t div256_x2 = ((uint64_t)sys_hz * 256ull * 2ull) / ((uint64_t)mclk_freq * 5ull);
    uint32_t div256 = (uint32_t)((div256_x2 + 1ull) >> 1);
    mclk_pio_div_int  = (uint16_t)(div256 >> 8);
    mclk_pio_div_frac = (uint8_t)(div256 & 0xFFu);
    pio_sm_set_clkdiv_int_frac(pico_audio.pio_1, pico_audio.sm_mclk,
                               mclk_pio_div_int, mclk_pio_div_frac);

    /* achieved MCLK from the rounded divider */
    uint64_t actual_x256 = ((uint64_t)sys_hz * 256ull * 2ull) / (5ull * (uint64_t)div256);
    mclk_actual_hz = (uint32_t)((actual_x256 + 1ull) >> 1);
    int64_t err = (int64_t)mclk_actual_hz - (int64_t)mclk_freq;
    mclk_error_ppm = (int32_t)((err * 1000000ll) / (int64_t)mclk_freq);
}

void dout_pio_init(void)
{
    pio_sm_claim(pico_audio.pio_2, pico_audio.sm_dout);
    uint offset = pio_add_program(pico_audio.pio_2, &audio_pio_program);
    audio_pio_program_init(pico_audio.pio_2, pico_audio.sm_dout, offset,
                           pico_audio.audio_dout, pico_audio.audio_lrclk);
    pio_sm_set_clkdiv(pico_audio.pio_2, pico_audio.sm_dout, 1.0f);
    pio_sm_set_enabled(pico_audio.pio_2, pico_audio.sm_dout, true);
}

void din_pio_init(void)
{
    pio_sm_claim(pico_audio.pio_1, pico_audio.sm_din);
    uint offset = pio_add_program(pico_audio.pio_1, &read_pio_program);
    read_pio_program_init(pico_audio.pio_1, pico_audio.sm_din, offset,
                          pico_audio.audio_din, pico_audio.audio_lrclk);
    pio_sm_set_clkdiv(pico_audio.pio_1, pico_audio.sm_din, 1.0f);
    pio_sm_set_enabled(pico_audio.pio_1, pico_audio.sm_din, true);
}

void mclk_pio_init(void)
{
    pio_sm_claim(pico_audio.pio_1, pico_audio.sm_mclk);
    uint offset = pio_add_program(pico_audio.pio_1, &mclk_pio_program);
    mclk_pio_program_init(pico_audio.pio_1, pico_audio.sm_mclk, offset,
                          pico_audio.audio_mclk);
    set_mclk_frequency(pico_audio.mclk_freq);
    pio_sm_set_enabled(pico_audio.pio_1, pico_audio.sm_mclk, true);
}
