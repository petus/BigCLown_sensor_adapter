/*
*
*   Sensor adapter - BMP180 example code (temperature)
*
*   This example code demonstrates the settings and communication 
*   with temperature and pressure sensor BMP180 (0x77) and 
*   popular IoT platform called BigClown.
*
*   Created by Petus (2018)
*   www.chiptron.cz
*   www.time4ee.com
*
*/

#include <application.h>

// Multiplying instead of dividing - faster
#define BMP180_1_16     ((float) 0.0625)
#define BMP180_1_256    ((float) 0.00390625)
#define BMP180_1_2048   ((float) 0.00048828125)
#define BMP180_1_4096   ((float) 0.000244140625)
#define BMP180_1_8192   ((float) 0.0001220703125)
#define BMP180_1_32768  ((float) 0.000030517578125)
#define BMP180_1_65536  ((float) 0.0000152587890625)
#define BMP180_1_101325 ((float) 0.00000986923266726)

// LED instance
bc_led_t led;

bc_i2c_transfer_t rx_transfer;
bc_i2c_transfer_t rx_coef_transfer;

bc_i2c_transfer_t tx_coef_transfer; 
bc_i2c_transfer_t tx_req_transfer;
bc_i2c_transfer_t tx_temp_transfer;

uint8_t tx_req_buffer[] = {0xF4, 0x2E}; // write 0x2E to 0xF4 register - measurement request
uint8_t tx_temp_buffer[] = {0xF6};      // temperature register (0xF6 MSB, 0xF7 LSB)
uint8_t tx_coef_buffer[] = {0xAA};      // the first address of coefs
uint8_t rx_coef_buffer[22] = {0x00};    // coef buffer

// EEPROM values
int16_t AC1, AC2, AC3, B1, B2, MB, MC, MD;
uint16_t AC4, AC5, AC6;

int32_t X1, X2, X3, B3, B5, B6;
uint32_t B4, B7;

void application_init(void)
{
    uint16_t i = 0;

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, false);
    bc_led_set_mode(&led, BC_LED_MODE_ON);

    // Initialize UART2
    bc_uart_init(BC_UART_UART2, BC_UART_BAUDRATE_115200, BC_UART_SETTING_8N1);

    // Initialize I2C1
    bc_i2c_init(BC_I2C_I2C1, BC_I2C_SPEED_100_KHZ);

    /********************************************
    *                                           *
    *               Read all coefs              *
    *                                           *
    *********************************************/

    tx_coef_transfer.device_address = 0x77;         // slave address
    tx_coef_transfer.buffer = tx_coef_buffer;       // write 0xAA - coef address
    tx_coef_transfer.length = sizeof(tx_coef_buffer);

    bc_i2c_write(BC_I2C_I2C1, &tx_coef_transfer);   // write

    rx_coef_transfer.device_address = 0x77;         // slave address
    rx_coef_transfer.buffer = rx_coef_buffer;       // rx buffer of coef
    rx_coef_transfer.length = sizeof(rx_coef_buffer);

    bc_i2c_read(BC_I2C_I2C1, &rx_coef_transfer);    // read

    // save configuration values
	AC1 = (int16_t)(rx_coef_buffer[i] << 8 | rx_coef_buffer[i + 1]); i += 2;
	AC2 = (int16_t)(rx_coef_buffer[i] << 8 | rx_coef_buffer[i + 1]); i += 2;
	AC3 = (int16_t)(rx_coef_buffer[i] << 8 | rx_coef_buffer[i + 1]); i += 2;
	AC4 = (uint16_t)(rx_coef_buffer[i] << 8 | rx_coef_buffer[i + 1]); i += 2;
	AC5 = (uint16_t)(rx_coef_buffer[i] << 8 | rx_coef_buffer[i + 1]); i += 2;
	AC6 = (uint16_t)(rx_coef_buffer[i] << 8 | rx_coef_buffer[i + 1]); i += 2;
	B1 = (int16_t)(rx_coef_buffer[i] << 8 | rx_coef_buffer[i + 1]); i += 2;
	B2 = (int16_t)(rx_coef_buffer[i] << 8 | rx_coef_buffer[i + 1]); i += 2;
	MB = (int16_t)(rx_coef_buffer[i] << 8 | rx_coef_buffer[i + 1]); i += 2;
	MC = (int16_t)(rx_coef_buffer[i] << 8 | rx_coef_buffer[i + 1]); i += 2;
    MD = (int16_t)(rx_coef_buffer[i] << 8 | rx_coef_buffer[i + 1]);
}

void application_task(void)
{
    uint8_t rx_buffer[2] = {0x00};
    float temperature = 0x00;
    char str[50] = {0x00};

    // Toggle LED
    bc_led_set_mode(&led, BC_LED_MODE_TOGGLE);

    /********************************************
    *                                           *
    *               Temperature                 *
    *                                           *
    *********************************************/

    tx_req_transfer.device_address = 0x77;          // slave address
    tx_req_transfer.buffer = tx_req_buffer;         // write 0x2E to 0xF4 register
    tx_req_transfer.length = sizeof(tx_req_buffer);

    bc_i2c_write(BC_I2C_I2C1, &tx_req_transfer);    // write

    // wait at least 4.5ms
    for(uint32_t i = 0; i < 0xFFFF; i++)
    {
        ; // do nothing
    }

    tx_temp_transfer.device_address = 0x77;         // slave address
    tx_temp_transfer.buffer = tx_temp_buffer;       // read temperature from 0xF6 (MSB) and 0xF7 (LCB) register
    tx_temp_transfer.length = sizeof(tx_temp_buffer);

    bc_i2c_write(BC_I2C_I2C1, &tx_temp_transfer);   // write

    rx_transfer.device_address = 0x77;              // slave address
    rx_transfer.buffer = rx_buffer;                 // rx buffer - temperature
    rx_transfer.length = sizeof(rx_buffer);

    bc_i2c_read(BC_I2C_I2C1, &rx_transfer);         // read

    temperature = ((rx_buffer[0] << 8) | rx_buffer[1]);    // sum MSB and LSB values

    // calculate true temperature
	X1 = (temperature - AC6) * AC5 * BMP180_1_32768;
	X2 = MC * 2048 / (X1 + MD);
	B5 = X1 + X2;
	
	// temperature in Celsius
    temperature = (B5 + 8) / ((float)160);

    sprintf(str, "Temperature: %.2f *C \n", temperature);

    bc_uart_write(BC_UART_UART2, &str, strlen(str)); // send rx buffer through UART2


    // Plan next run this function after 1000 ms
    bc_scheduler_plan_current_from_now(1000);
}
