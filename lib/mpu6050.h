#ifndef MPU6050_H
#define MPU6050_H

#include "hardware/i2c.h"
#include "pico/stdlib.h"

// Endereço padrão do MPU6050
#define MPU6050_DEFAULT_ADDR 0x68

// Inicializa o MPU6050 (reset e wake)
void mpu6050_init(i2c_inst_t *i2c, uint8_t addr);

// Reseta o MPU6050
void mpu6050_reset(i2c_inst_t *i2c, uint8_t addr);

// Lê os dados brutos do acelerômetro, giroscópio e temperatura
void mpu6050_read_raw(i2c_inst_t *i2c, uint8_t addr, int16_t accel[3], int16_t gyro[3], int16_t *temp);

#endif
