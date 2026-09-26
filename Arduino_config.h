#ifndef _ARDUINO_CONFIG_H_
#define _ARDUINO_CONFIG_H_

// Pico board configuration used by the Arduino compatibility layer.
// Keep all code-configured peripheral instances and GPIO assignments here.

// UART used by Serial / SimpleFOC Commander.
#define CONFIG_ARDUINO_UART_PORT_NUM uart0
#define CONFIG_ARDUINO_UART_IRQ UART0_IRQ
#define CONFIG_ARDUINO_UART_TXD 0
#define CONFIG_ARDUINO_UART_RXD 1

// I2C master.
#define CONFIG_ARDUINO_I2C_HOST_NUM i2c0
#define CONFIG_ARDUINO_I2C_SCL 5
#define CONFIG_ARDUINO_I2C_SDA 4
#define CONFIG_ARDUINO_I2C_FREQ_HZ 400000

// SPI master used by the magnetic encoder.
#define CONFIG_ARDUINO_SPI_HOST_NUM spi0
#define CONFIG_ARDUINO_SPI_MOSI 19
#define CONFIG_ARDUINO_SPI_MISO 16
#define CONFIG_ARDUINO_SPI_CLK 18
#define CONFIG_ARDUINO_SPI_CS0 17
#define CONFIG_ARDUINO_SPI_BAUD_RATE (10 * 1000 * 1000)

// ADC input GPIO channels.
#define CONFIG_ARDUINO_ADC_GPIO_0 26
#define CONFIG_ARDUINO_ADC_GPIO_1 27
#define CONFIG_ARDUINO_ADC_GPIO_2 28

// Three-phase FOC driver GPIOs.
#define CONFIG_FOC_MOTOR_U 10
#define CONFIG_FOC_MOTOR_V 11
#define CONFIG_FOC_MOTOR_W 12
#define CONFIG_FOC_MOTOR_EN 13

#endif
