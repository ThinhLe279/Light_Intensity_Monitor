/*
 This code tests NUCLEO-L152RE board ADC inputs PA0,PA1 (A0,A1) with UART.
 */

/* Includes */
#include <stddef.h>
#include "stm32l1xx.h"
#include "nucleo152start.h"
#define HSI_VALUE    ((uint32_t)16000000)
#include <stdio.h>
#include <math.h>

#define SLAVE_ADDR 0x04
#define START_ADDR 0x01

void USART2_Init(void);
void USART2_write(char data);
char USART2_read(void);

void USART1_Init(void);
void USART1_write(char data);
char USART1_read();

void delay_Ms(int delay);

float read_LUX(void);

unsigned short int CRC16(char *nData, unsigned short int wLength);

int check_input_reg(char rec);

float findMedian(float *array, int n);

void Response_frame(int senor_median_value);

void wrong_slave(void);

int Mflag = 0;

/**
 **===========================================================================
 **
 **  Abstract: main program
 **
 **===========================================================================
 */
int main(void) {
	/* Configure the system clock to 32 MHz and update SystemCoreClock */
	__disable_irq();
	USART2_Init();
	USART1_Init();
	SetSysClock();
	SystemCoreClockUpdate();
	/* TODO - Add your application code here */
	USART1->CR1 |= 0x0020;			//enable RX interrupt
	NVIC_EnableIRQ(USART1_IRQn); 	//enable interrupt in NVIC
	__enable_irq();
	//set up pin PA5 for LED
	RCC->AHBENR |= 1;				//enable GPIOA clock
	GPIOA->MODER &= ~0x00000C00;	//clear pin mode
	GPIOA->MODER |= 0x00000400;		//set pin PA5 to output model
	//GPIOA->ODR^=0x20;				//0010 0000 xor bit 5. p186
	//GPIOA->ODR^=0x20;				//0010 0000 xor bit 5. p186
	delay_Ms(1000);
	char Received_frame[8] = { 0 };
	uint16_t wlength = 8;

	//set up pin PA0 and PA1 for analog input

	GPIOA->MODER |= 0x3;			//PA0 analog (A0)
	GPIOA->MODER |= 0xC;			//PA1 analog (A1)
	delay_Ms(1000);
	//setup ADC1. p272
	RCC->APB2ENR |= 0x00000200;		//enable ADC1 clock
	ADC1->CR2 = 0;					//bit 1=0: Single conversion mode
	ADC1->SMPR3 = 7;		//384 cycles sampling time for channel 0 (longest)
	ADC1->CR1 &= ~0x03000000;		//resolution 12-bit

	/* Infinite loop */

	//USART1->CR1 &= ~0x08;   // disable TE pin of USART1
	int LEN = 9;
	float sensor_median_value = 0;
	float sensor_arr[9] = { 0 };
	int cast = 0;
	int check = 0;
	char sensor_high_byte = 0;
	char sensor_low_byte = 0;
	while (1) {

		switch (Mflag) {
		case 0:
			for (int i = 0; i < wlength; i++) {
				Received_frame[i] = '0';
			}
			break;

		case 1:

			Received_frame[0] = SLAVE_ADDR;
			for (int i = 1; i < wlength; i++) {
				Received_frame[i] = USART1_read();// take the remaining 7 bytes of the request frame
			}
			for (int i = 0; i < wlength; i++) {
				USART1_write(Received_frame[i]);// display request frame received from master

			}
			for (int i = 0; i < wlength; i++) {
				USART2_write(Received_frame[i]);		// display for debugging

			}

			unsigned short int CRC2 = CRC16(Received_frame, wlength);
			USART1_write(CRC2);
			USART2_write(CRC2);				// debugging
			delay_Ms(10);
			if (CRC2 == 0) {

				check = check_input_reg(Received_frame[3]);

				if (check) {

					for (int i = 0; i < LEN; i++) {
						sensor_arr[i] = read_LUX();
						//USART2_write(sensor_arr[i]);
						delay_Ms(100);
					}

					//delay_Ms(10);
					sensor_median_value = findMedian(sensor_arr, LEN);
					sensor_median_value *= 10;
					cast = (int) sensor_median_value;

					sensor_high_byte = (cast >> 8) | sensor_high_byte;
					sensor_low_byte = cast | sensor_low_byte;
					//USART2_write(sensor_high_byte);
					//USART2_write(sensor_low_byte);
					Response_frame(cast);
					//delay_Ms(10);

				}
			}

			Mflag = 0;
			USART1->CR1 |= 0x0020;			//enable RX interrupt

			break;

		case 2:
			wrong_slave();
			break;
		}

	}
	return 0;
}

void wrong_slave(void) {

	USART1->CR1 &= ~0x00000004;		//RE bit. p739-740. Disable receiver
	delay_Ms(10); 					//time=1/9600 x 10 bits x 7 byte = 7,29 ms
	USART1->CR1 |= 0x00000004;		//RE bit. p739-740. Enable receiver
	USART1->CR1 |= 0x0020;			//enable RX interrupt
	Mflag = 0;
}

float read_LUX(void) {
	//char buf[100];

	ADC1->SQR5 = 0;				//conversion sequence starts at ch0
	ADC1->CR2 |= 1;				//bit 0, ADC on/off (1=on, 0=off)
	ADC1->CR2 |= 0x40000000;		//start conversion

	int result = 0;
	float lux = 0;

	while (!(ADC1->SR & 2)) {
	}	//wait for conversion complete
	result = ADC1->DR;	//read conversion result

	if ((result > 0) && (result < 400)) {

		lux = (-13.427 * result) + 5999.8;
	} else if ((result > 400) && (result < 1000)) {

		lux = (-0.8761 * result) + 1243.7;

	} else if ((result > 1000) && (result < 2000)) {

		lux = (-0.5371 * result) + 900.01;

	} else if ((result > 2000) && (result < 4000)) {

		lux = (-0.0385 * result) + 136;

	}
	delay_Ms(100);

	//ADC1->CR2&=~1;	//bit 0, ADC on/off (1=on, 0=off)
	return lux;

}

void USART1_Init(void) {
	RCC->APB2ENR |= (1 << 14);	 	//set bit 14 (USART1 EN) p.156
	RCC->AHBENR |= 0x00000001; 	//enable GPIOA port clock bit 0 (GPIOA EN)
	GPIOA->AFR[1] = 0x00000700;	//GPIOx_AFRL p.189,AF7 p.177
	GPIOA->AFR[1] |= 0x00000070;	//GPIOx_AFRL p.189,AF7 p.177
	GPIOA->MODER |= 0x00080000; //MODER2=PA9(TX)D8 to mode 10=alternate function mode. p184
	GPIOA->MODER |= 0x00200000; //MODER2=PA10(RX)D2 to mode 10=alternate function mode. p184

	USART1->BRR = 0x00000D05;	//9600 BAUD and crystal 32MHz. p710, D05
	USART1->CR1 = 0x00000008;	//TE bit. p739-740. Enable transmit
	USART1->CR1 |= 0x00000004;	//RE bit. p739-740. Enable receiver
	USART1->CR1 |= 0x00002000;	//UE bit. p739-740. Uart enable
}

void USART2_Init(void) {

	RCC->APB1ENR |= 0x00020000; 	//set bit 17 (USART2 EN)
	RCC->AHBENR |= 0x00000001; 	//enable GPIOA port clock bit 0 (GPIOA EN)
	GPIOA->AFR[0] = 0x00000700;	//GPIOx_AFRL p.188,AF7 p.177
	GPIOA->AFR[0] |= 0x00007000;	//GPIOx_AFRL p.188,AF7 p.177
	GPIOA->MODER |= 0x00000020; //MODER2=PA2(TX) to mode 10=alternate function mode. p184
	GPIOA->MODER |= 0x00000080; //MODER2=PA3(RX) to mode 10=alternate function mode. p184

	USART2->BRR = 0x00000D05;	//11500 BAUD and crystal 32MHz. p710, 116
	USART2->CR1 = 0x00000008;	//TE bit. p739-740. Enable transmit
	USART2->CR1 |= 0x00000004;	//RE bit. p739-740. Enable receiver
	USART2->CR1 |= 0x00002000;	//UE bit. p739-740. Uart enable
}

void USART1_write(char data) {
	//wait while TX buffer is empty
	while (!(USART1->SR & 0x0080)) {
	} 	//TXE: Transmit data register empty. p736-737
	USART1->DR = (data);			//p739
}
void USART2_write(char data) {
	//wait while TX buffer is empty
	while (!(USART2->SR & 0x0080)) {
	} 	//TXE: Transmit data register empty. p736-737
	USART2->DR = (data);		//p739
}

char USART1_read() {
	char data = 0;
	//wait while RX buffer is data is ready to be read
	while (!(USART1->SR & 0x0020)) {
	} 	//Bit 5 RXNE: Read data register not empty
	data = USART1->DR;			//p739
	return data;
}
char USART2_read() {
	char data = 0;
	//wait while RX buffer data is ready to be read
	while (!(USART2->SR & 0x0020)) {
	} //Bit 5 RXNE: Read data register not empty
	data = USART2->DR;			//p739
	return data;
}

void delay_Ms(int delay) {
	int i = 0;
	for (; delay > 0; delay--)
		for (i = 0; i < 2460; i++)
			; //measured with oscilloscope
}
void USART1_IRQHandler(void) {

	char received_addr = 0;
	//char buf[20];

	//This bit is set by hardware when the content of the
	//RDR shift register has been transferred to the USART_DR register.
	if (USART1->SR & 0x0020) 		//if data available in DR register. p737
			{
		received_addr = USART1->DR;
		//USART_write(received_addr);
	}
	if (received_addr == SLAVE_ADDR) {
		Mflag = 1;
		//USART2_write(received_addr);

	}

	else {
		Mflag = 2;
		//USART2_write(received_addr);
	}

	USART1->CR1 &= ~0x0020;	//Disable USARTx interrupt (RXNEIE interrupt disable)
}

unsigned short int CRC16(char *nData, unsigned short int wLength) {
	uint8_t i, j;
	unsigned short int crc = 0xFFFF;
	for (i = 0; i < wLength; i++) {
		crc ^= nData[i];
		for (j = 0; j < 8; j++) {
			if (crc & 1) {
				crc = (crc >> 1) ^ 0xA001;
			} else {
				crc = crc >> 1;
			}
		}
	}
	return crc;
}

int check_input_reg(char rec) {
	int check;
	if (rec == START_ADDR) {
		check = 1;

	} else
		check = 0;
	return check;
}

float findMedian(float *array, int n) {
	int i, j;
	float median;
	// sort the array
	for (i = 0; i < n - 1; i++) {
		for (j = 0; j < n - i - 1; j++) {
			if (array[j] > array[j + 1]) {
				float temp = array[j];
				array[j] = array[j + 1];
				array[j + 1] = temp;
			}
		}
	}
	// calculate the median
	if (n % 2 == 0) {
		median = (array[(n - 1) / 2] + array[n / 2]) / 2.0;
	} else {
		median = array[n / 2];
	}
	return median;
}

void Response_frame(int sensor_median_value) {

	GPIOA->ODR |= 0x20;     // LED on while transmitting control RE/TE
	//USART1->CR1 &= ~0x04;   // disable RE pin of USART1
	//USART1->CR1 = 0x08;		// re-enable TE pin of USART1

	char response_frame[7] = { SLAVE_ADDR, 0x04, 0x02, 0, 0, 0, 0 };
	char sensor_high_byte = 0;
	char sensor_low_byte = 0;
	unsigned short int calculated_CRC = 0;
	char crc_high_byte = 0;
	char crc_low_byte = 0;

	sensor_high_byte = (sensor_median_value >> 8) | sensor_high_byte;
	sensor_low_byte = sensor_median_value | sensor_low_byte;

	response_frame[3] = sensor_high_byte;
	response_frame[4] = sensor_low_byte;

	calculated_CRC = CRC16(response_frame, 5);
	crc_high_byte = (calculated_CRC >> 8) | crc_high_byte;
	crc_low_byte = calculated_CRC | crc_low_byte;

	response_frame[6] = crc_high_byte;
	response_frame[5] = crc_low_byte;

	for (int i = 0; i < 7; i++) {

		USART1_write(response_frame[i]);
	}

	for (int i = 0; i < 7; i++) {

		USART2_write(response_frame[i]);
	}

	GPIOA->ODR &= ~0x20; 		// LED off, turn to receiving mode
	//USART1->CR1 &= ~0x08;   // disable TE pin of USART1
	//USART1->CR1 |= 0x04 ;  	// re-enable RE pin of USART1

}

