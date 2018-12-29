/*
*
*   Sensor adapter - Si7021 example code (temperature)
*
*   This example code demonstrates the settings and communication 
*   with temperature and humidity sensor Si7021 and 
*   popular IoT platform called BigClown.
*
*   Created by Petus (2018)
*   www.chiptron.cz
*   www.time4ee.com
*
*/

#include <application.h>

// LED instance
bc_led_t led;

bc_i2c_transfer_t rx_transfer;
bc_i2c_transfer_t tx_transfer;
uint8_t tx_buffer[] = {0xE3}; // temperature

void application_init(void)
{
    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_ON);

    // Initialize UART2
    bc_uart_init(BC_UART_UART2, BC_UART_BAUDRATE_115200, BC_UART_SETTING_8N1);

    // Initialize I2C1
    bc_i2c_init(BC_I2C_I2C1, BC_I2C_SPEED_100_KHZ);
}

void application_task(void)
{
    uint8_t rx_buffer[2] = {0x00};
    float temperature = 0x00;
    char str[50] = {0x00};

    // Toggle LED
    bc_led_set_mode(&led, BC_LED_MODE_TOGGLE);

    /*
    *
    * Si7021 - temperature & humidity sensor 
    * 7-bit slave address - 0x40
    * request for temperature - 0xE3
    * 
    */

    /*  TEMPERATURE  */

    tx_transfer.device_address = 0x40;   // slave address
    tx_transfer.buffer = tx_buffer;      // 0xE3
    tx_transfer.length = sizeof(tx_buffer);

    bc_i2c_write(BC_I2C_I2C1, &tx_transfer); // write

    rx_transfer.device_address = 0x40;   // slave address
    rx_transfer.buffer = rx_buffer;      // rx buffer
    rx_transfer.length = sizeof(rx_buffer);

    bc_i2c_read(BC_I2C_I2C1, &rx_transfer); // read

    temperature = ((rx_buffer[0] << 8) | rx_buffer[1]);    // sum MSB and LSB values

	temperature = ((175.72 * temperature)/65536) - 46.85;   // formula from datasheet

    sprintf(str, "Temperature: %.2f *C \n", temperature);

    bc_uart_write(BC_UART_UART2, &str, strlen(str)); // send rx buffer through UART2


    // Plan next run this function after 1000 ms
    bc_scheduler_plan_current_from_now(1000);
}
