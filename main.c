#include <DAVE.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#define TEST_SUCCESS 1
#define TEST_FAILED 0

typedef enum MPC9809
{
	ADR = 0x18 << 1, 			//MPC9809 address
	INIT_REG = 0x00,
	REG_CONF = 0x01 << 1, 		// config. register
	TEMP_REG = 0x05,			//temp reg
	SHUT_UP_DOWN = 0x0100 << 1 	//sensor shut up and down
} TEMP_SENS;

char mb[200]; // MessageBuffer for sprintf()

void delay(uint32_t);

volatile uint8_t tx_completion_0 = 0, rx_completion_0 = 0;

int main(void)
{
	uint32_t i2c_master_fifo_xfer_status = 0;

	DAVE_STATUS_t status;

	status = DAVE_Init();

	if(status != DAVE_STATUS_SUCCESS)
	{
		while(true){}
	}

	USBD_VCOM_Connect();

	while(!USBD_VCOM_IsEnumDone());
	while(!cdc_event_flags.line_encoding_event_flag);

	delay(0xffff);

	while (true)
	{
		uint8_t i2c_data_x[1];
		uint8_t i2c_temp = INIT_REG;

		I2C_MASTER_Init(&I2C_MASTER_0);

		I2C_MASTER_Transmit(&I2C_MASTER_0, true, ADR, &i2c_temp, 1, true);
		while (tx_completion_0 == 0);
		tx_completion_0 = 0;

		i2c_temp = TEMP_REG;

		I2C_MASTER_Transmit(&I2C_MASTER_0, true, ADR, &i2c_temp, 1, false);
		while (tx_completion_0 == 0);
		tx_completion_0 = 0;

		I2C_MASTER_Receive(&I2C_MASTER_0,true, ADR, i2c_data_x, 2, true, true);
		while(rx_completion_0 == 0);
		rx_completion_0 = 0;

		if (!i2c_master_fifo_xfer_status)
		{
			i2c_master_fifo_xfer_status = TEST_SUCCESS;
		}
		else
		{
			i2c_master_fifo_xfer_status = TEST_FAILED;
		}

		float temp = 0;

		i2c_data_x[0] = i2c_data_x[0] & 0x1f;

		if ((i2c_data_x[0] & 0x10) == 0x10)
		{
			i2c_data_x[0] = i2c_data_x[1] & 0x0f;
			temp = 256 - ((i2c_data_x[0]*16) + (i2c_data_x[1]/16));
		}
		else
		{
			temp = (i2c_data_x[0]*16.0) + (i2c_data_x[1]/16.0);
		}

		sprintf(mb, "T1 = %d || T2 = %d\n", i2c_data_x[0], i2c_data_x[1]);
		USBD_VCOM_SendString((int8_t*)mb);
		CDC_Device_USBTask(&USBD_VCOM_cdc_interface);

		sprintf(mb, "%f \n", temp);
		USBD_VCOM_SendString((int8_t*)mb);
		CDC_Device_USBTask(&USBD_VCOM_cdc_interface);

		delay(0xffffff);
	}
	return 1;
}

void delay(uint32_t cnt)
{
	volatile uint32_t count = cnt;
	while (--count)
	{

	}
}

void EndOfTransmit(void)
{
	tx_completion_0 = 1;
}

void EndOfReceive(void)
{
	rx_completion_0 = 1;
}
