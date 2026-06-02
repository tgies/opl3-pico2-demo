/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <string.h>
#include "es8311.h"
#include "DEV_Config.h"


#ifndef BIT
#define BIT(nr)             (1 << (nr))
#endif

/* ES8311 address
* 0x32:CE=1;0x30:CE=0
* 0x32>>1 = 0x19
* 0x30>>1 = 0x18
*/
#define ES8311_ADDR         0x18

/*
* to define the clock soure of MCLK
*/
#define FROM_MCLK_PIN       1
#define FROM_SCLK_PIN       0

/*
* to define whether to reverse the clock
*/
#define INVERT_MCLK         0 // do not invert
#define INVERT_SCLK         0

#define IS_DMIC             0 // Is it a digital microphone

/*
 * Clock coefficient structure
 */
struct _coeff_div {
    uint32_t mclk;        /* mclk frequency */
    uint32_t rate;        /* sample rate */
    uint8_t pre_div;      /* the pre divider with range from 1 to 8 */
    uint8_t pre_multi;    /* the pre multiplier with 0: 1x, 1: 2x, 2: 4x, 3: 8x selection */
    uint8_t adc_div;      /* adcclk divider */
    uint8_t dac_div;      /* dacclk divider */
    uint8_t fs_mode;      /* double speed or single speed, =0, ss, =1, ds */
    uint8_t lrck_h;       /* adclrck divider and daclrck divider */
    uint8_t lrck_l;
    uint8_t bclk_div;     /* sclk divider */
    uint8_t adc_osr;      /* adc osr */
    uint8_t dac_osr;      /* dac osr */
};

/* codec hifi mclk clock divider coefficients */
static const struct _coeff_div coeff_div[] = {
    /*!<mclk     rate   pre_div  mult  adc_div dac_div fs_mode lrch  lrcl  bckdiv osr */
    /* 8k */
    {12288000, 8000, 0x06, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 8000, 0x03, 0x01, 0x03, 0x03, 0x00, 0x05, 0xff, 0x18, 0x10, 0x10},
    {16384000, 8000, 0x08, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000, 8000, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 8000, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {4096000, 8000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 8000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2048000, 8000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 8000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1024000, 8000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 11.025k */
    {11289600, 11025, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800, 11025, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400, 11025, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200, 11025, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 12k */
    {12288000, 12000, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 12000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 12000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 12000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 16k */
    {12288000, 16000, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 16000, 0x03, 0x01, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x10},
    {16384000, 16000, 0x04, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000, 16000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 16000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {4096000, 16000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 16000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2048000, 16000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 16000, 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1024000, 16000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 22.05k */
    {11289600, 22050, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800, 22050, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400, 22050, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200, 22050, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {705600, 22050, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 24k */
    {12288000, 24000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 24000, 0x03, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 24000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x08, 0x10, 0x10},
    {3072000, 24000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 24000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 32k */
    {12288000, 32000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 32000, 0x03, 0x02, 0x03, 0x03, 0x00, 0x02, 0xff, 0x0c, 0x10, 0x10},
    {16384000, 32000, 0x02, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000, 32000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 32000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {4096000, 32000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 32000, 0x03, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2048000, 32000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 32000, 0x03, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},
    {1024000, 32000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 44.1k */
    {11289600, 44100, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800, 44100, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400, 44100, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200, 44100, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 48k */
    {12288000, 48000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 48000, 0x03, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 48000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 48000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 48000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},

    /* 64k */
    {12288000, 64000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 64000, 0x03, 0x02, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
    {16384000, 64000, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {8192000, 64000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 64000, 0x01, 0x02, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
    {4096000, 64000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 64000, 0x01, 0x03, 0x03, 0x03, 0x01, 0x01, 0x7f, 0x06, 0x10, 0x10},
    {2048000, 64000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 64000, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0xbf, 0x03, 0x18, 0x18},
    {1024000, 64000, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

    /* 88.2k */
    {11289600, 88200, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {5644800, 88200, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {2822400, 88200, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1411200, 88200, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},

    /* 96k */
    {12288000, 96000, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {18432000, 96000, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {6144000, 96000, 0x01, 0x02, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {3072000, 96000, 0x01, 0x03, 0x01, 0x01, 0x00, 0x00, 0xff, 0x04, 0x10, 0x10},
    {1536000, 96000, 0x01, 0x03, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x02, 0x10, 0x10},
};

void es8311_write_reg(uint8_t reg_addr, uint8_t data)
{
    return DEV_I2C_Write(ES8311_ADDR, reg_addr, data);
}

uint8_t es8311_read_reg(uint8_t reg_addr)
{
    return DEV_I2C_ReadByte(ES8311_ADDR, reg_addr);
}

/*
* look for the coefficient in coeff_div[] table
*/
static int get_coeff(uint32_t mclk, uint32_t rate)
{
    for (int i = 0; i < (sizeof(coeff_div) / sizeof(coeff_div[0])); i++) {
        if (coeff_div[i].rate == rate && coeff_div[i].mclk == mclk) {
            return i;
        }
    }

    return -1;
}

int es8311_sample_frequency_config(int mclk_frequency, int sample_frequency)
{
    uint8_t regv;

    /* Get clock coefficients from coefficient table */
    int coeff = get_coeff(mclk_frequency, sample_frequency);

    if (coeff < 0) {
        printf("Unable to configure sample rate %dHz with %dHz MCLK", sample_frequency, mclk_frequency);
        return -1;
    }

    const struct _coeff_div *const selected_coeff = &coeff_div[coeff];

    /* register 0x02 */
    regv = es8311_read_reg(ES8311_CLK_MANAGER_REG02);
    regv &= 0x07;
    regv |= (selected_coeff->pre_div - 1) << 5;
    regv |= selected_coeff->pre_multi << 3;
    es8311_write_reg(ES8311_CLK_MANAGER_REG02, regv);

    /* register 0x03 */ 
    const uint8_t reg03 = (selected_coeff->fs_mode << 6) | selected_coeff->adc_osr;
    es8311_write_reg(ES8311_CLK_MANAGER_REG03, reg03);

    /* register 0x04 */
    es8311_write_reg(ES8311_CLK_MANAGER_REG04, selected_coeff->dac_osr);

    /* register 0x05 */
    const uint8_t reg05 = ((selected_coeff->adc_div - 1) << 4) | (selected_coeff->dac_div - 1);
    es8311_write_reg(ES8311_CLK_MANAGER_REG05, reg05);

    /* register 0x06 */
    regv = es8311_read_reg(ES8311_CLK_MANAGER_REG06);
    regv &= 0xE0;

    if (selected_coeff->bclk_div < 19) {
        regv |= (selected_coeff->bclk_div - 1) << 0;
    } else {
        regv |= (selected_coeff->bclk_div) << 0;
    }

    es8311_write_reg(ES8311_CLK_MANAGER_REG06, regv);

    /* register 0x07 */
    regv = es8311_read_reg(ES8311_CLK_MANAGER_REG07);
    regv &= 0xC0;
    regv |= selected_coeff->lrck_h << 0;
    es8311_write_reg(ES8311_CLK_MANAGER_REG07, regv);

    /* register 0x08 */
    es8311_write_reg(ES8311_CLK_MANAGER_REG08, selected_coeff->lrck_l);

    return 0;
}

static void es8311_clock_config(pico_audio_t pico_audio)
{
    uint8_t reg06;
    uint8_t reg01 = 0x3F; // Enable all clocks
    int mclk_hz;

    /* Select clock source for internal MCLK and determine its frequency */
    if (FROM_MCLK_PIN) {
        mclk_hz = pico_audio.mclk_freq;
    } else {
        mclk_hz = pico_audio.sample_freq * (int)pico_audio.res_out * 2;
        reg01 |= BIT(7); // Select BCLK (a.k.a. SCK) pin
    }

    if (INVERT_MCLK) {
        reg01 |= BIT(6); // Invert MCLK pin
    }
    es8311_write_reg(ES8311_CLK_MANAGER_REG01, reg01);

    reg06 = es8311_read_reg(ES8311_CLK_MANAGER_REG06);
    if (INVERT_SCLK) {
        reg06 |= BIT(5);
    } else {
        reg06 &= ~BIT(5);
    }
    reg06 |= 0x03; //BCLK divider at master mode
    es8311_write_reg(ES8311_CLK_MANAGER_REG06, reg06);

    /* Configure clock dividers */
    es8311_sample_frequency_config(mclk_hz, pico_audio.sample_freq);
}

static int es8311_resolution_config(const es8311_resolution_t res, uint8_t *reg)
{
    switch (res) {
    case ES8311_RESOLUTION_16:
        *reg |= (3 << 2);
        break;
    case ES8311_RESOLUTION_18:
        *reg |= (2 << 2);
        break;
    case ES8311_RESOLUTION_20:
        *reg |= (1 << 2);
        break;
    case ES8311_RESOLUTION_24:
        *reg |= (0 << 2);
        break;
    case ES8311_RESOLUTION_32:
        *reg |= (4 << 2);
        break;
    default:
        return -1;
    }
    return 0;
}

static int es8311_fmt_config(pico_audio_t pico_audio)
{
    uint8_t reg09 = 0; // SDP In
    uint8_t reg0a = 0; // SDP Out

    printf("ES8311 in Master mode and I2S format\n");
    uint8_t reg00;
    reg00 = es8311_read_reg(ES8311_RESET_REG00);
    reg00 |= 0x40;
    es8311_write_reg(ES8311_RESET_REG00, reg00); // Master serial port - default

    /* Setup SDP In and Out resolution */
    es8311_resolution_config(pico_audio.res_in, &reg09);
    es8311_resolution_config(pico_audio.res_out, &reg0a);

    es8311_write_reg(ES8311_SDPIN_REG09, reg09);
    es8311_write_reg(ES8311_SDPOUT_REG0A, reg0a);

    return 0;
}

void es8311_microphone_config()
{
    uint8_t reg14 = 0x1A; // enable analog MIC and max PGA gain

    /* PDM digital microphone enable or disable */
    if (IS_DMIC) {
        reg14 |= BIT(6);
    }
    es8311_write_reg(ES8311_ADC_REG17, 0xFF); // Set ADC gain @todo move this to ADC config section

    return es8311_write_reg(ES8311_SYSTEM_REG14, reg14);
}

void es8311_init(pico_audio_t pico_audio)
{
    /* Reset ES8311 to its default */
    es8311_write_reg(ES8311_RESET_REG00, 0x1F);
    DEV_Delay_ms(20);
    es8311_write_reg(ES8311_RESET_REG00, 0x00);
    es8311_write_reg(ES8311_RESET_REG00, 0x80); // Power-on command

    /* Setup clock: source, polarity and clock dividers */
    es8311_clock_config(pico_audio);

    /* Setup audio format (fmt): master/slave, resolution, I2S */
    es8311_fmt_config(pico_audio);

    es8311_write_reg(ES8311_SYSTEM_REG0D, 0x01); // Power up analog circuitry - NOT default
    es8311_write_reg(ES8311_SYSTEM_REG0E, 0x02); // Enable analog PGA, enable ADC modulator - NOT default
    es8311_write_reg(ES8311_SYSTEM_REG12, 0x00); // power-up DAC - NOT default
    es8311_write_reg(ES8311_SYSTEM_REG13, 0x10); // Enable output to HP drive - NOT default
    es8311_write_reg(ES8311_ADC_REG1C, 0x6A); // ADC Equalizer bypass, cancel DC offset in digital domain
    es8311_write_reg(ES8311_DAC_REG37, 0x08); // Bypass DAC equalizer - NOT default
}

int es8311_voice_volume_set(int volume)
{
    if (volume < 0) {
        volume = 0;
    } else if (volume > 100) {
        volume = 100;
    }

    int reg32;
    if (volume == 0) {
        reg32 = 0;
    } else {
        reg32 = ((volume) * 256 / 100) - 1;
    }

    es8311_write_reg(ES8311_DAC_REG32, reg32);

    // provide user with real volume set
    return volume;
}

int es8311_voice_volume_get()
{
    uint8_t reg32,volume;
    reg32 = es8311_read_reg(ES8311_DAC_REG32);

    if (reg32 == 0) {
        volume = 0;
    } else {
        volume = ((reg32 * 100) / 256) + 1;
    }
    return volume;
}

void es8311_voice_mute(bool mute)
{
    uint8_t reg31;
    reg31 = es8311_read_reg(ES8311_DAC_REG31);

    if (mute) {
        reg31 |= BIT(6) | BIT(5);
    } else {
        reg31 &= ~(BIT(6) | BIT(5));
    }

    return es8311_write_reg(ES8311_DAC_REG31, reg31);
}

void es8311_microphone_gain_set(es8311_mic_gain_t gain_db)
{
    return es8311_write_reg(ES8311_ADC_REG16, gain_db); // ADC gain scale up
}

void es8311_voice_fade(const es8311_fade_t fade)
{
    uint8_t reg37;
    reg37 = es8311_read_reg(ES8311_DAC_REG37);
    reg37 &= 0x0F;
    reg37 |= (fade << 4);
    return es8311_write_reg(ES8311_DAC_REG37, reg37);
}

void es8311_microphone_fade(const es8311_fade_t fade)
{
    uint8_t reg15;
    reg15 = es8311_read_reg(ES8311_ADC_REG15);
    reg15 &= 0x0F;
    reg15 |= (fade << 4);
    return es8311_write_reg(ES8311_ADC_REG15, reg15);
}

void es8311_register_dump()
{
    for (int reg = 0; reg < 0x4A; reg++) {
        uint8_t value;
        value = es8311_read_reg(reg);
        printf("REG:%02x: %02x", reg, value);
    }
}

uint16_t es8311_read_id()
{
    uint8_t chip_id_LSB = es8311_read_reg(0xFD);
    uint8_t chip_id_MSB = es8311_read_reg(0xFE); 
    uint16_t chip_id = (chip_id_MSB << 8) + chip_id_LSB;
    return chip_id;
}