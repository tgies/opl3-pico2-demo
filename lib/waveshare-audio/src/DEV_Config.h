/* Minimal hardware shim used by es8311.c. Provides I2C bring-up,
 * blocking I2C read/write, and ms delays. Original by Waveshare; this
 * trimmed version drops SPI, PWM, key-config, etc. (none used by ES8311).
 */

#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT        i2c1
#define ES8311_SDA_PIN  34
#define ES8311_SCL_PIN  35
#define PA_CTRL         17

#ifdef __cplusplus
extern "C" {
#endif

uint8_t DEV_Module_Init(void);
void DEV_I2C_Write(uint8_t addr, uint8_t reg, uint8_t value);
uint8_t DEV_I2C_ReadByte(uint8_t addr, uint8_t reg);
void DEV_Delay_ms(uint32_t xms);
void DEV_PA_Set(uint8_t enable);

#ifdef __cplusplus
}
#endif

#endif
