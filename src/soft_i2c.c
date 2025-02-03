#include "soft_i2c.h"
#include "stm32f4xx_hal.h"

// Helper functions to set/reset GPIO
#define SCL_HIGH() HAL_GPIO_WritePin(SOFT_I2C_SCL_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_SET)
#define SCL_LOW()  HAL_GPIO_WritePin(SOFT_I2C_SCL_PORT, SOFT_I2C_SCL_PIN, GPIO_PIN_RESET)
#define SDA_HIGH() HAL_GPIO_WritePin(SOFT_I2C_SDA_PORT, SOFT_I2C_SDA_PIN, GPIO_PIN_SET)
#define SDA_LOW()  HAL_GPIO_WritePin(SOFT_I2C_SDA_PORT, SOFT_I2C_SDA_PIN, GPIO_PIN_RESET)
#define SDA_READ() HAL_GPIO_ReadPin(SOFT_I2C_SDA_PORT, SOFT_I2C_SDA_PIN)


#if SOFT_I2C_USE_DWT_DELAY
void DWT_Init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  // Enable DWT
    DWT->CYCCNT = 0;  // Reset cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;  // Enable cycle counter
}
void DWT_Delay_us(uint32_t us) {
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = (HAL_RCC_GetHCLKFreq() / 1000000) * us; // Convert us to CPU cycles
    while ((DWT->CYCCNT - start) < ticks);
}
static void I2C_Delay(void) {
    DWT_Delay_us(5);  // Adjust to match desired I2C speed
}
#else //SOFT_I2C_USE_DWT_DELAY
// Small delay
static void I2C_Delay(void) {
    for (volatile int i = 0; i < 10; i++);  // Adjust timing if needed
}
#endif //SOFT_I2C_USE_DWT_DELAY


// Initialize I2C GPIOs
void Soft_I2C_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable GPIO Clock
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Configure SCL and SDA as open-drain outputs
    GPIO_InitStruct.Pin = SOFT_I2C_SCL_PIN | SOFT_I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SOFT_I2C_SCL_PORT, &GPIO_InitStruct);

    // Set default idle state
    SCL_HIGH();
    SDA_HIGH();

    #if SOFT_I2C_USE_DWT_DELAY
    DWT_Init();
    #else //SOFT_I2C_USE_DWT_DELAY
    #endif //SOFT_I2C_USE_DWT_DELAY
}

// Generate START condition
void Soft_I2C_Start(void) {
    SDA_HIGH();
    SCL_HIGH();
    I2C_Delay();
    SDA_LOW();
    I2C_Delay();
    SCL_LOW();
}

// Generate STOP condition
void Soft_I2C_Stop(void) {
    SCL_LOW();
    SDA_LOW();
    I2C_Delay();
    SCL_HIGH();
    I2C_Delay();
    SDA_HIGH();
}

// Write 8-bit data
void Soft_I2C_Write(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80)  // MSB first
            SDA_HIGH();
        else
            SDA_LOW();
        I2C_Delay();
        SCL_HIGH();
        I2C_Delay();
        SCL_LOW();
        data <<= 1;
    }
    Soft_I2C_Wait_ACK();
}

// Read 8-bit data
uint8_t Soft_I2C_Read(uint8_t ack) {
    uint8_t data = 0;

    SDA_HIGH();  // Release SDA for input
    for (uint8_t i = 0; i < 8; i++) {
        SCL_HIGH();
        I2C_Delay();
        data = (data << 1) | (SDA_READ() ? 1 : 0);
        SCL_LOW();
        I2C_Delay();
    }

    if (ack)
        Soft_I2C_Send_ACK();
    else
        Soft_I2C_Send_NACK();

    return data;
}

// Wait for ACK
uint8_t Soft_I2C_Wait_ACK(void) {
    SDA_HIGH();  // Release SDA
    I2C_Delay();
    SCL_HIGH();
    I2C_Delay();
    
    uint8_t ack = SDA_READ();
    SCL_LOW();
    
    return ack == 0;
}

// Send ACK
void Soft_I2C_Send_ACK(void) {
    SDA_LOW();
    I2C_Delay();
    SCL_HIGH();
    I2C_Delay();
    SCL_LOW();
}

// Send NACK
void Soft_I2C_Send_NACK(void) {
    SDA_HIGH();
    I2C_Delay();
    SCL_HIGH();
    I2C_Delay();
    SCL_LOW();
}

// Multi-byte write function
uint8_t i2c_write(uint8_t i2cAdd, uint8_t regAdd, uint8_t *data, uint32_t dataLen) {
    Soft_I2C_Start();
    
    Soft_I2C_Write(i2cAdd << 1);  // Send device address with write bit (0)
    if (!Soft_I2C_Wait_ACK()) {
        Soft_I2C_Stop();
        return 0;  // Error
    }

    Soft_I2C_Write(regAdd);  // Send register address
    if (!Soft_I2C_Wait_ACK()) {
        Soft_I2C_Stop();
        return 0;  // Error
    }

    for (uint32_t i = 0; i < dataLen; i++) {
        Soft_I2C_Write(data[i]);  // Write data byte
        if (!Soft_I2C_Wait_ACK()) {
            Soft_I2C_Stop();
            return 0;  // Error
        }
    }

    Soft_I2C_Stop();
    return 1;  // Success
}

// Multi-byte read function
uint8_t i2c_read(uint8_t i2cAdd, uint8_t regAdd, uint8_t *data, uint32_t dataLen) {
    Soft_I2C_Start();

    Soft_I2C_Write(i2cAdd << 1);  // Send device address with write bit (0)
    if (!Soft_I2C_Wait_ACK()) {
        Soft_I2C_Stop();
        return 0;  // Error
    }

    Soft_I2C_Write(regAdd);  // Send register address
    if (!Soft_I2C_Wait_ACK()) {
        Soft_I2C_Stop();
        return 0;  // Error
    }

    Soft_I2C_Start();
    Soft_I2C_Write((i2cAdd << 1) | 1);  // Send device address with read bit (1)
    if (!Soft_I2C_Wait_ACK()) {
        Soft_I2C_Stop();
        return 0;  // Error
    }

    for (uint32_t i = 0; i < dataLen; i++) {
        data[i] = Soft_I2C_Read(i < (dataLen - 1));  // Send ACK for all but the last byte
    }

    Soft_I2C_Stop();
    return 1;  // Success
}

uint8_t Soft_I2C_IsDeviceReady(uint8_t i2cAdd) {
    Soft_I2C_Start();
    Soft_I2C_Write(i2cAdd << 1);  // Send address with write bit (0)

    uint8_t ack = Soft_I2C_Wait_ACK();  // Check if device responds

    Soft_I2C_Stop();
    
    return ack;  // Return 1 if device is ready, 0 if not
}
