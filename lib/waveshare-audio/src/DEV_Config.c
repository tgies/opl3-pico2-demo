/* Minimal hardware shim used by es8311.c. */

#include "DEV_Config.h"

uint8_t DEV_Module_Init(void)
{
    /* PA enable (NS4150B amp). Driven HIGH later by DEV_PA_Set(1) after the
     * codec is brought up, to avoid a power-on click. */
    gpio_init(PA_CTRL);
    gpio_set_dir(PA_CTRL, GPIO_OUT);
    gpio_put(PA_CTRL, 0);

    /* Shared I2C bus on i2c1 @ 100 kHz. Codec at 0x18, but every other I2C
     * device on the board also lives here (AXP2101, RTC, IMU, touch, cam). */
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(ES8311_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(ES8311_SCL_PIN, GPIO_FUNC_I2C);
    gpio_set_pulls(ES8311_SDA_PIN, true, false);
    gpio_set_pulls(ES8311_SCL_PIN, true, false);

    return 0;
}

void DEV_I2C_Write(uint8_t addr, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    i2c_write_blocking(I2C_PORT, addr, data, 2, false);
}

uint8_t DEV_I2C_ReadByte(uint8_t addr, uint8_t reg)
{
    uint8_t buf = 0;
    i2c_write_blocking(I2C_PORT, addr, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, addr, &buf, 1, false);
    return buf;
}

void DEV_Delay_ms(uint32_t xms)
{
    sleep_ms(xms);
}

void DEV_PA_Set(uint8_t enable)
{
    gpio_put(PA_CTRL, enable ? 1 : 0);
}
