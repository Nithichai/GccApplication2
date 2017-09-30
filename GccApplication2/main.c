/*
 * GccApplication2.c
 *
 * Created: 9/27/2017 12:52:16 PM
 * Author : nithi
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include FOSC 16000000
#define BAUD 9600
#define MYUBRR ((FOSC/16/BAUD)-1)
volatile uint16_t time_push;
volatile uint16_t Capt1;
/*
 * Flag 0 = Send trigger
 * Flag 1 = Detect rising
 * Flag 2 = Count time and detect falling
 * Flag 3 = Send to computer
 * Flag 4 = Ack from computer and reset
 */
volatile uint8_t Flag = 0;

void SetFlag0 (void)
{
	/* Set normal mode */
	TCCR1A = 0x00;
	/* Set prescaler 8 */
	TCCR1B = 0x02;
	/* Interrupt timer overflow */
	TIMSK1 = 0x01;
	/* Set time 10 us */
	TCNT1 = 65516;
	/* Start interrupt */
	sei();
	/* Set portB is output */
	DDRB = 0xFF;
}

void SetFlag1 (void) 
{
	/* Set normal mode */
	TCCR1A = 0x00;
	/* Set prescaler 1024 and set rising edge detect */
	TCCR1B = 0x45;
	/* Interrupt capture and timer overflow */
	TIMSK1 = 0x21;
	/* Start interrupt */
	sei();
	/* Set portB is output */
	DDRB = 0x00;
}

void SetFlag2(void)
{
	/* Set normal mode */
	TCCR1A = 0x00;
	/* Set prescaler 1024 and set falling edge detect */
	TCCR1B = 0x05;
	/* Interrupt capture and timer overflow */
	TIMSK1 = 0x21;
	/* Start interrupt */
	sei();
	/* Set portB is output */
	DDRB = 0x00;
}

void SetFlag3(void)
{
	/* Set normal mode */
	TCCR1A = 0x00;
	/* Set prescaler 1024 and set rising edge detect */
	TCCR1B = 0x45;
	/* Interrupt timer overflow */
	TIMSK1 = 0x01;
	/* Start interrupt */
	sei();
	/* Set portB is output */
	DDRB = 0x00;
} 

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

void USART_Init( unsigned int ubrr) {
	/*Set baud rate */
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0A &= 0xFD;
	/* Enable receiver and transmitter */
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	/* Set frame format: 8data, 2stop bit */
	UCSR0C = (3<<UCSZ00); 
}

ISR(TIMER1_CAPT_vect) 
{
	if (Flag == 1)
	{
		SetFlag2();
		Flag++;
	}
	if (Flag == 2)
	{
		Capt1 = ICR1;
		SetFlag3();
		Flag++;
	}
}

ISR(TIMER1_OVF_vect) 
{
	if (Flag == 0) 
	{
		SetFlag1();
		Flag++;
	}
}

int main(void)
{	
	SetFlag0();
	USART_Init(MYUBRR);
	while(1) {
		if (Flag == 3) 
		{
			time_push = Capt1;
			USART_Transmit((char) time_push);
			char recv = USART_Receive();
			if (recv == 'O') 
			{
				Flag++;
			}
		}
		if (Flag == 4)
		{
			Capt1 = 0;
			Flag = 0;
			SetFlag0();
		}
	}
}