#include "soft_i2c.h"

int main(void) {
    HAL_Init();
    SystemClock_Config();
    
    Soft_I2C_Init();
    UART_Init();  // Needed for printf debugging
    
    uint8_t deviceAddr = 0x50;  // Example I2C device address

    if (Soft_I2C_IsDeviceReady(deviceAddr)) {
        printf("Device at 0x%02X is READY!\r\n", deviceAddr);
    } else {
        printf("Device at 0x%02X is NOT responding!\r\n", deviceAddr);
    }
    
    uint8_t dataToWrite[2] = {0x55, 0xAA};
    uint8_t dataToRead[2];

    i2c_write(0x50, 0x10, dataToWrite, 2);  // Write 2 bytes to register 0x10
    HAL_Delay(10);
    i2c_read(0x50, 0x10, dataToRead, 2);  // Read 2 bytes from register 0x10

    while (1);
}
