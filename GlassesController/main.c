#define F_CPU 8000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/power.h>
#include <avr/sfr_defs.h>
#define BAUD 19200
#include <util/setbaud.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>

#define true 1
#define false 0

#define SPI_SS                     PB2
#define SPI_SS_PORT                PORTB
#define SPI_SS_PIN                 PINB
#define SPI_SS_DDR                 DDRB

#define SPI_MOSI                     PB3
#define SPI_MOSI_PORT                PORTB
#define SPI_MOSI_PIN                 PINB
#define SPI_MOSI_DDR                 DDRB

#define SPI_MISO                     PB4
#define SPI_MISO_PORT                PORTB
#define SPI_MISO_PIN                 PINB
#define SPI_MISO_DDR                 DDRB

#define SPI_SCK                     PB5
#define SPI_SCK_PORT                PORTB
#define SPI_SCK_PIN                 PINB
#define SPI_SCK_DDR                 DDRB

#define SLAVE_SELECT    SPI_SS_PORT &= ~(1 << SPI_SS)
#define SLAVE_DESELECT  SPI_SS_PORT |= (1 << SPI_SS)

#define CONFIG_A0_MSB 0b10010101 // start conversion / A0_A3 / ±2.048 / single shot
#define CONFIG_A1_MSB 0b10100101 // start conversion / A1_A3 / ±2.048 / single shot
#define CONFIG_A2_MSB 0b10110101 // start conversion / A2_A3 / ±2.048 / single shot
const uint8_t CONFIG_MSB_A[3] = {CONFIG_A0_MSB, CONFIG_A1_MSB, CONFIG_A2_MSB};
#define CONFIG_LSB 0b11101011 // 860 SPS / ADC mode / Pull-up enable / valid data / not used

#define LED_GLASS_DDR DDRC
#define LED_GLASS_PORT PORTC
#define GLASS_LEFT PORTC1
#define GLASS_RIGHT PORTC2
#define LED_LEFT PORTC3
#define LED_RIGTH PORTC4
const uint8_t LED_LIST[2] = {LED_LEFT, LED_RIGTH};
	
#define NOTIFY_DDR DDRB
#define NOTIFY_PORT PORTB
#define NOTIFY_LED PORTB0
	
void Turn_On(uint8_t Port_num){
	LED_GLASS_PORT |= (1 << Port_num);
}

void Turn_Off(uint8_t port_num){
	LED_GLASS_PORT &= ~(1 << port_num);
}	
	
void UpdateGlasses(){
	if (ACSR & (1<<ACO)){
		Turn_On(GLASS_LEFT);
		Turn_Off(GLASS_RIGHT);
	}
	else{
		Turn_On(GLASS_RIGHT);
		Turn_Off(GLASS_LEFT);
	}
}	
	
#define busy_loop_until_bit_is_clear(sfr, bit) do {UpdateGlasses();} while (bit_is_set(sfr, bit))
#define busy_loop_until_bit_is_set(sfr, bit) do {UpdateGlasses();} while (bit_is_clear(sfr, bit))

void busy_delay(){
	for(int i = 0; i < 200; i++){
		UpdateGlasses();
		_delay_us(10);
	}
}

void initUSART(void) {                                /* requires BAUD */
	UBRR0H = UBRRH_VALUE;                        /* defined in setbaud.h */
	UBRR0L = UBRRL_VALUE;
	#if USE_2X
	UCSR0A |= (1 << U2X0);
	#else
	UCSR0A &= ~(1 << U2X0);
	#endif
	/* Enable USART transmitter/receiver */
	UCSR0B = (1 << TXEN0) | (1 << RXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);   /* 8 data bits, 1 stop bit */
}

uint8_t receiveByte(void) {
	busy_loop_until_bit_is_set(UCSR0A, RXC0);
	return UDR0;                                // return register value
}

void transmitByte(uint8_t data) {
	/* Wait for empty transmit buffer */
	busy_loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = data;                                            /* send data */
}

void transmitWord(uint16_t data){
	transmitByte(data >> 8); // MSB first
	transmitByte(data % 256);
}

void printWord(uint16_t word) {
	transmitByte('0' + (word / 10000));                 /* Ten-thousands */
	transmitByte('0' + ((word / 1000) % 10));               /* Thousands */
	transmitByte('0' + ((word / 100) % 10));                 /* Hundreds */
	transmitByte('0' + ((word / 10) % 10));                      /* Tens */
	transmitByte('0' + (word % 10));                             /* Ones */
}

void printBinaryByte(uint8_t byte) {
	/* Prints out a byte as a series of 1's and 0's */
	uint8_t bit;
	for (bit = 7; bit < 255; bit--) {
		if (bit_is_set(byte, bit))
		transmitByte('1');
		else
		transmitByte('0');
	}
}

void printString(const char myString[]) {
	uint8_t i = 0;
	while (myString[i]) {
		transmitByte(myString[i]);
		i++;
	}
}

void initSPI(void) {
	SPI_SS_DDR |= (1 << SPI_SS);                        /* set SS output */
	SPI_SS_PORT |= (1 << SPI_SS);       /* start off not selected (high) */

	SPI_MOSI_DDR |= (1 << SPI_MOSI);                   /* output on MOSI */
	SPI_MISO_PORT |= (1 << SPI_MISO);                  /* pull_up on MISO */
	SPI_SCK_DDR |= (1 << SPI_SCK);                      /* output on SCK */

	SPCR |= (1 << CPHA);				/* SPI mode 1 */
	SPCR |= (1 << SPR1);                /* div 16, safer for breadboards */
	SPCR |= (1 << MSTR);                                  /* clock_master */
	SPCR |= (1 << SPE);                                        /* enable */
}

void SPI_tradeByte(uint8_t byte) {
	SPDR = byte;                       /* SPI starts sending immediately */
	busy_loop_until_bit_is_set(SPSR, SPIF);                /* wait until done */
	/* SPDR now contains the received byte */
}

void ADC_Convert(uint8_t sensor_configure_command){
	//start conversion
	SLAVE_SELECT;
	SPI_tradeByte(sensor_configure_command);
	SPI_tradeByte(CONFIG_LSB);
	SPI_tradeByte(0);
	SPI_tradeByte(0);
	SLAVE_DESELECT;
	busy_delay();
}

uint16_t ADC_Result(){
	uint8_t sensor_data_msb, sensor_data_lsb;
	uint16_t sensor_data;
	//read result
	SLAVE_SELECT;
	SPI_tradeByte(0);
	sensor_data_msb = SPDR;
	SPI_tradeByte(0);
	sensor_data_lsb = SPDR;
	SPI_tradeByte(0);
	SPI_tradeByte(0);
	SLAVE_DESELECT;
	
	sensor_data = sensor_data_msb;
	sensor_data = sensor_data << 8;
	sensor_data += sensor_data_lsb;
	
	return sensor_data;
}

int main(void)
{
	clock_prescale_set(clock_div_1);
	LED_GLASS_DDR |= (1 << GLASS_LEFT) | (1 << GLASS_RIGHT) | (1 << LED_LEFT) | (1 << LED_RIGTH);
	NOTIFY_DDR |= (1 << NOTIFY_LED);
	
	initSPI();
	initUSART();
	
	while (receiveByte() != 's');
	for(int i = 0; i < 8; i++){
		NOTIFY_PORT ^= (1 << NOTIFY_LED);
		_delay_ms(100);
	}
	
	uint16_t sensor_data_off[3], sensor_data_on[3];

	while (true){
		while (receiveByte() != 'c');
		//start conversions
		for(int led = 0; led < 2; led++){
			for(int sensor = 0; sensor < 3; sensor++){
				ADC_Convert(CONFIG_MSB_A[sensor]); // led off
				Turn_On(LED_LIST[led]);
				sensor_data_off[sensor] = ADC_Result();
				ADC_Convert(CONFIG_MSB_A[sensor]); // led on
				Turn_Off(LED_LIST[led]);
				sensor_data_on[sensor] = ADC_Result();
			}
			//send led result
			for(int sensor = 0; sensor < 3; sensor++){
				transmitWord(sensor_data_off[sensor]);
				transmitWord(sensor_data_on[sensor]);
			}
		}
	}
}