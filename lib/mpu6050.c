#include "mpu6050.h"

void mpu6050_init(i2c_inst_t *i2c, uint8_t addr)
{
    uint8_t buf[] = {0x6B, 0x80}; // Reset
    i2c_write_blocking(i2c, addr, buf, 2, false);
    sleep_ms(100);

    buf[1] = 0x00; // Wake
    i2c_write_blocking(i2c, addr, buf, 2, false);
    sleep_ms(10);
}

void mpu6050_reset(i2c_inst_t *i2c, uint8_t addr)
{
    uint8_t buf[] = {0x6B, 0x80};
    i2c_write_blocking(i2c, addr, buf, 2, false);
    sleep_ms(100);
    buf[1] = 0x00;
    i2c_write_blocking(i2c, addr, buf, 2, false);
    sleep_ms(10);
}

void mpu6050_read_raw(i2c_inst_t *i2c, uint8_t addr, int16_t accel[3], int16_t gyro[3], int16_t *temp)
{
    uint8_t buffer[6];
    uint8_t val = 0x3B;
    i2c_write_blocking(i2c, addr, &val, 1, true);
    i2c_read_blocking(i2c, addr, buffer, 6, false);
    for (int i = 0; i < 3; i++)
        accel[i] = (buffer[i * 2] << 8) | buffer[(i * 2) + 1];

    val = 0x43;
    i2c_write_blocking(i2c, addr, &val, 1, true);
    i2c_read_blocking(i2c, addr, buffer, 6, false);
    for (int i = 0; i < 3; i++)
        gyro[i] = (buffer[i * 2] << 8) | buffer[(i * 2) + 1];

    val = 0x41;
    i2c_write_blocking(i2c, addr, &val, 1, true);
    i2c_read_blocking(i2c, addr, buffer, 2, false);
    *temp = (buffer[0] << 8) | buffer[1];
}
