#ifndef SOFT_I2C_H
#define SOFT_I2C_H

#include "stm32f4xx_hal.h"

#define SOFT_I2C_USE_DWT_DELAY 1

// Define GPIO pins for SDA and SCL
#define SOFT_I2C_SCL_PORT   GPIOB
#define SOFT_I2C_SCL_PIN    GPIO_PIN_6
#define SOFT_I2C_SDA_PORT   GPIOB
#define SOFT_I2C_SDA_PIN    GPIO_PIN_7

// Function prototypes
void Soft_I2C_Init(void);
void Soft_I2C_Start(void);
void Soft_I2C_Stop(void);
void Soft_I2C_Write(uint8_t data);
uint8_t Soft_I2C_Read(uint8_t ack);
uint8_t Soft_I2C_Wait_ACK(void);
void Soft_I2C_Send_ACK(void);
void Soft_I2C_Send_NACK(void);

// High-level I2C functions for multi-byte communication
uint8_t i2c_write(uint8_t i2cAdd, uint8_t regAdd, uint8_t *data, uint32_t dataLen);
uint8_t i2c_read(uint8_t i2cAdd, uint8_t regAdd, uint8_t *data, uint32_t dataLen);

#endif
