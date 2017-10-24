/*
 * GccApplication2.c
 *
 * Created: 9/27/2017 12:52:16 PM
 * Author : nithi
 */
#define F_CPU 16000000L
#define BAUD 9600
#define MYUBRR ((F_CPU/16/BAUD)-1)
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <math.h>

volatile uint16_t length;
// volatile uint16_t Capt1;
/*
 * Flag 0 = Send trigger
 * Flag 1 = Detect rising
 * Flag 2 = Count time and detect falling
 * Flag 3 = Send to computer
 * Flag 4 = Reset
 */
volatile uint8_t Flag;
volatile uint8_t triggerState;

unsigned char USART_Receive(void) {
	/* Wait for data to be received */
	while ( !(UCSR0A & (1<<RXC0)) );
	/* Get and return received data from buffer */
	return UDR0;
}

void USART_Transmit(unsigned char data) {
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) );
	/* Put data into buffer, sends the data */
	UDR0 = data;
}

void USART_Init(unsigned int ubrr) {
	/*Set baud rate */
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0A &= 0xFD;
	/* Enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	/* Set frame format: 8data, 2stop bit */
	UCSR0C = (3<<UCSZ00);
}

void Start(void)
{
	length = 0;
	Flag = 0;
	triggerState = 0;
	TCCR1A = 0x00;
	TIMSK1 = 0x21;
	sei();
}

void RestartValue(void)
{
	length = 0;
	Flag = 0;
	triggerState = 0;	
}

/*  */
void SendTrigger (void)
{
	//USART_Transmit('T');
	//USART_Transmit('\n');
	/* Set time 10 us */
	TCNT1 = 65516;
	/* Set prescaler 8 */
	TCCR1B = 0x02;
	/* Set portB is output */
	DDRB = 0x02;
	/* Set trigger high */
	PORTB = 0x02;
}

void DetectRising (void) 
{
	// USART_Transmit('R');
	// USART_Transmit('\n');
	/* Set time = */
	TCNT1 = 0;
	/* Set normal mode */
	// TCCR1A = 0x00;
	/* Set prescaler 1024 and set rising edge detect */
	TCCR1B = 0x45;
	/* Interrupt capture */
	// TIMSK1 = 0x21;
	/* Start interrupt */
	// sei();
}

void DetectFalling(void)
{
	//USART_Transmit('F');
	//USART_Transmit('\n');
	/* Set time = */
	TCNT1 = 0;
	/* Set normal mode */
	//TCCR1A = 0x00;
	/* Set prescaler 1024 and set falling edge detect */
	TCCR1B = 0x05;
	/* Interrupt capture */
	// TIMSK1 = 0x21;
	/* Start interrupt */
	//sei();
}

void SendAllNumber(volatile uint16_t number)
{
	int n = 5; //log10(number) + 1;
	int i = 0;
	int data[n];
	for (i = 0; i < n; i++, number /= 10)
	{
		data[i] = number % 10;
	}
	for (i = n-1; i >= 0; i--)
	{
		USART_Transmit(data[i] + '0');
	}
	USART_Transmit('\n');
}

void SendValue(volatile uint16_t number)
{
	// USART_Transmit('V');
	// USART_Transmit(':');
	/* Set normal mode */
	// TCCR1A = 0x00;
	/* Set no clock and set falling edge detect */
	TCCR1B = 0x00;
	/* no interrupt */
	TIMSK1 = 0x00;
	SendAllNumber(number);
}

ISR(TIMER1_CAPT_vect) 
{
	if (Flag == 1)
	{
		/*USART_Transmit(':');
		USART_Transmit('1');
		USART_Transmit('\n');*/
		DetectFalling();
		Flag = 2;
	}
	else if (Flag == 2)
	{
		length = ICR1;
		if (length < 64)
		{
			length = length * 1024 / 16 / 58;
		} 
		else if (length < 1024)
		{
			length = length / 16 * 1024 / 58;
		}
		else if (length < 3712)
		{
			length = length / 58 * 1024 / 16;
		} 
		else 
		{ 
			length = length / 16 / 58 * 1024;
		}
		length = ICR1; //ICR1 * 1024 / 16 / 58;
		/*USART_Transmit(':');
		USART_Transmit('2');
		USART_Transmit('\n');*/
		SendValue(length);
		_delay_ms(500);
		Start();
		SendTrigger();
	} 
	else 
	{
		USART_Transmit('C');
		USART_Transmit('A');
		USART_Transmit('P');
		USART_Transmit('T');
		USART_Transmit('>');
		USART_Transmit(Flag + '0');
		USART_Transmit('\n');
		Start();
		SendTrigger();
	}
}

ISR(TIMER1_OVF_vect) 
{
	/*USART_Transmit('O');
	USART_Transmit('V');
	USART_Transmit('F');*/
	if (Flag == 0) 
	{
		/*USART_Transmit(':');
		USART_Transmit('0');
		USART_Transmit('\n');*/
		PORTB = 0x00;
		triggerState = 1;
		DetectRising();
		Flag = 1;
	} 
	else 
	{
		USART_Transmit('O');
		USART_Transmit('V');
		USART_Transmit('F');
		USART_Transmit('>');
		USART_Transmit('X');
		USART_Transmit('\n');
		Start();
		SendTrigger();
	}
}

int main(void)
{	
	// DelayRestart();
	Start();
	USART_Init(MYUBRR);
	SendTrigger();
	while(1) {
		/*USART_Transmit(Flag + '0');
		USART_Transmit(triggerState + '0');
		USART_Transmit('\n');
		_delay_ms(200);*/
	}
}