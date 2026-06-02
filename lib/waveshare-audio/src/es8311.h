#ifndef _ES8311_H_
#define _ES8311_H_

#include "audio_pio.h"

/*
 *   ES8311_REGISTER NAME_REG_REGISTER ADDRESS
 */
#define ES8311_RESET_REG00              0x00  /*reset digital,csm,clock manager etc.*/

/*
 * Clock Scheme Register definition
 */
#define ES8311_CLK_MANAGER_REG01        0x01 /* select clk src for mclk, enable clock for codec */
#define ES8311_CLK_MANAGER_REG02        0x02 /* clk divider and clk multiplier */
#define ES8311_CLK_MANAGER_REG03        0x03 /* adc fsmode and osr  */
#define ES8311_CLK_MANAGER_REG04        0x04 /* dac osr */
#define ES8311_CLK_MANAGER_REG05        0x05 /* clk divier for adc and dac */
#define ES8311_CLK_MANAGER_REG06        0x06 /* bclk inverter and divider */
#define ES8311_CLK_MANAGER_REG07        0x07 /* tri-state, lrck divider */
#define ES8311_CLK_MANAGER_REG08        0x08 /* lrck divider */
/*
 * SDP
 */
#define ES8311_SDPIN_REG09              0x09 /* dac serial digital port */
#define ES8311_SDPOUT_REG0A             0x0A /* adc serial digital port */
/*
 * SYSTEM
 */
#define ES8311_SYSTEM_REG0B             0x0B /* system */
#define ES8311_SYSTEM_REG0C             0x0C /* system */
#define ES8311_SYSTEM_REG0D             0x0D /* system, power up/down */
#define ES8311_SYSTEM_REG0E             0x0E /* system, power up/down */
#define ES8311_SYSTEM_REG0F             0x0F /* system, low power */
#define ES8311_SYSTEM_REG10             0x10 /* system */
#define ES8311_SYSTEM_REG11             0x11 /* system */
#define ES8311_SYSTEM_REG12             0x12 /* system, Enable DAC */
#define ES8311_SYSTEM_REG13             0x13 /* system */
#define ES8311_SYSTEM_REG14             0x14 /* system, select DMIC, select analog pga gain */
/*
 * ADC
 */
#define ES8311_ADC_REG15                0x15 /* ADC, adc ramp rate, dmic sense */
#define ES8311_ADC_REG16                0x16 /* ADC */
#define ES8311_ADC_REG17                0x17 /* ADC, volume */
#define ES8311_ADC_REG18                0x18 /* ADC, alc enable and winsize */
#define ES8311_ADC_REG19                0x19 /* ADC, alc maxlevel */
#define ES8311_ADC_REG1A                0x1A /* ADC, alc automute */
#define ES8311_ADC_REG1B                0x1B /* ADC, alc automute, adc hpf s1 */
#define ES8311_ADC_REG1C                0x1C /* ADC, equalizer, hpf s2 */
/*
 * DAC
 */
#define ES8311_DAC_REG31                0x31 /* DAC, mute */
#define ES8311_DAC_REG32                0x32 /* DAC, volume */
#define ES8311_DAC_REG33                0x33 /* DAC, offset */
#define ES8311_DAC_REG34                0x34 /* DAC, drc enable, drc winsize */
#define ES8311_DAC_REG35                0x35 /* DAC, drc maxlevel, minilevel */
#define ES8311_DAC_REG37                0x37 /* DAC, ramprate */
/*
 *GPIO
 */
#define ES8311_GPIO_REG44               0x44 /* GPIO, dac2adc for test */
#define ES8311_GP_REG45                 0x45 /* GP CONTROL */
/*
 * CHIP
 */
#define ES8311_CHD1_REGFD               0xFD /* CHIP ID1 */
#define ES8311_CHD2_REGFE               0xFE /* CHIP ID2 */
#define ES8311_CHVER_REGFF              0xFF /* VERSION */
#define ES8311_CHD1_REGFD               0xFD /* CHIP ID1 */

#define ES8311_MAX_REGISTER             0xFF
#define ES8311_I2C_ADDR                 0x18

/*************************************************************/

typedef enum {
    ES8311_MIC_GAIN_MIN = -1,
    ES8311_MIC_GAIN_0DB,
    ES8311_MIC_GAIN_6DB,
    ES8311_MIC_GAIN_12DB,
    ES8311_MIC_GAIN_18DB,
    ES8311_MIC_GAIN_24DB,
    ES8311_MIC_GAIN_30DB,
    ES8311_MIC_GAIN_36DB,
    ES8311_MIC_GAIN_42DB,
    ES8311_MIC_GAIN_MAX
} es8311_mic_gain_t;

typedef enum {
    ES8311_FADE_OFF = 0,
    ES8311_FADE_4LRCK, // 4LRCK means ramp 0.25dB/4LRCK
    ES8311_FADE_8LRCK,
    ES8311_FADE_16LRCK,
    ES8311_FADE_32LRCK,
    ES8311_FADE_64LRCK,
    ES8311_FADE_128LRCK,
    ES8311_FADE_256LRCK,
    ES8311_FADE_512LRCK,
    ES8311_FADE_1024LRCK,
    ES8311_FADE_2048LRCK,
    ES8311_FADE_4096LRCK,
    ES8311_FADE_8192LRCK,
    ES8311_FADE_16384LRCK,
    ES8311_FADE_32768LRCK,
    ES8311_FADE_65536LRCK
} es8311_fade_t;

typedef enum es8311_resolution_t {
    ES8311_RESOLUTION_16 = 16,
    ES8311_RESOLUTION_18 = 18,
    ES8311_RESOLUTION_20 = 20,
    ES8311_RESOLUTION_24 = 24,
    ES8311_RESOLUTION_32 = 32
} es8311_resolution_t;

/**
 * @brief Initialize ES8311
 *
 * There are two ways of providing Master Clock (MCLK) signal to ES8311 in Slave Mode:
 * 1. From MCLK pin:
 *    For flexible scenarios. A clock signal from I2S master is routed to MCLK pin.
 *    Its frequency must be defined in clk_cfg->mclk_frequency parameter.
 * 2. From SCLK pin:
 *    For simpler scenarios. ES8311 takes its clock from SCK pin. MCLK pin does not have to be connected.
 *    In this case, res_in must equal res_out; clk_cfg->mclk_frequency parameter is ignored
 *    and MCLK is calculated as MCLK = clk_cfg->sample_frequency * res_out * 2.
 *    Not all sampling frequencies are supported in this mode.
 *
 * @param dev ES8311 handle
 * @param[in] clk_cfg Clock configuration
 * @param[in] res_in  Input serial port resolution
 * @param[in] res_out Output serial port resolution
 * @return
 *     - ESP_OK success
 *     - ESP_ERR_INVALID_ARG Sample frequency or resolution invalid
 *     - Else fail
 */
void es8311_init(pico_audio_t pico_audio);

/**
 * @brief Set output volume
 *
 * Volume paramter out of <0, 100> interval will be truncated.
 *
 * @param dev ES8311 handle
 * @param[in] volume Set volume (0 ~ 100)
 * @param[out] volume_set Volume that was set. Same as volume, unless volume is outside of <0, 100> interval.
 *                        This parameter can be set to NULL, if user does not need this information.
 *
 * @return
 *     - ESP_OK success
 *     - Else fail
 */
int es8311_voice_volume_set(int volume);

/**
 * @brief Get output volume
 *
 * @param dev ES8311 handle
 * @param[out] volume get volume (0 ~ 100)
 *
 * @return
 *     - ESP_OK success
 *     - Else fail
 */
int es8311_voice_volume_get();

/**
 * @brief Print out ES8311 register content
 *
 * @param dev ES8311 handle
 */
void es8311_register_dump();

/**
 * @brief Mute ES8311 output
 *
 * @param dev ES8311 handle
 * @param[in] enable true: mute, false: don't mute
 * @return
 *     - ESP_OK success
 *     - Else fail
 */
void es8311_voice_mute(bool mute);

/**
 * @brief Set Microphone gain
 *
 * @param dev ES8311 handle
 * @param[in] gain_db Microphone gain
 * @return
 *     - ESP_OK success
 *     - Else fail
 */
void es8311_microphone_gain_set(es8311_mic_gain_t gain_db);

/**
 * @brief Configure microphone
 *
 * @param dev ES8311 handle
 * @param[in] digital_mic Set to true for digital microphone
 * @return
 *     - ESP_OK success
 *     - Else fail
 */
void es8311_microphone_config();

/**
 * @brief Configure sampling frequency
 *
 * @note This function is called by es8311_init().
 *       Call this function explicitly only if you want to change sample frequency during runtime.
 * @param dev ES8311 handle
 * @param[in] mclk_frequency   MCLK frequency in [Hz] (MCLK or SCLK pin, depending on bit register01[7])
 * @param[in] sample_frequency Required sample frequency in [Hz], e.g. 44100, 22050...
 * @return
 *     - ESP_OK success
 *     - ESP_ERR_INVALID_ARG cannot set clock dividers for given MCLK and sampling frequency
 *     - Else I2C read/write error
 */
int es8311_sample_frequency_config(int mclk_frequency, int sample_frequency);

/**
 * @brief Configure fade in/out for ADC: voice
 *
 * @param dev ES8311 handle
 * @param[in] fade Fade ramp rate
 * @return
 *     - ESP_OK success
 *     - Else I2C read/write error
 */
void es8311_voice_fade(const es8311_fade_t fade);

/**
 * @brief Configure fade in/out for DAC: microphone
 *
 * @param dev ES8311 handle
 * @param[in] fade Fade ramp rate
 * @return
 *     - ESP_OK success
 *     - Else I2C read/write error
 */
void es8311_microphone_fade(const es8311_fade_t fade);

uint16_t es8311_read_id();

#endif // !_ES8311_H_
