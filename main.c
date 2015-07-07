#define F_CPU           8000000UL              //8Mhz clock
#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))
#define CPU_8MHz       0x01
#define USART1BAUD      38400                  //USART Baud select
#define USART1UBRR      F_CPU/16/USART1BAUD-1   //USART UBRR calculation
#define ADC_PRESCALER	128

#define BAUD_RATE 38400

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <avr/cpufunc.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

static uint8_t aref = (1<<REFS0); // default to AREF = Vcc
// These buffers may be any size from 2 to 256 bytes.
#define RX_BUFFER_SIZE 64
#define TX_BUFFER_SIZE 40

static volatile uint8_t tx_buffer[TX_BUFFER_SIZE];
static volatile uint8_t tx_buffer_head;
static volatile uint8_t tx_buffer_tail;
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t rx_buffer_head;
static volatile uint8_t rx_buffer_tail;

//temp variables
char temp1 = 37;//100:1 ratio of this number to voltage
char temp2 = 56;//max is 256 = 2.56 volts
char temp3 = 115;
//timing
char phase1 = 10;
char phase2 = 10;
char phase3 = 10;
//misc
#define pw 1000 // Defines Pulse wave time in ms, currently set to 1s for testing, could reduce for increased accuracy

/*Port functions & connections:
PB0: Connects to H-Bridge, sends cold pulse
PB1: Connects to H-Bridge, sends hot pulses
PB3: LED always if in cycle function, PB3, PB4 and PB7 are phase indicators
PB4: LED on during phase two and three
PB7: LED on during phase three, LEDs start back over at only PB3 on after each cycle
PF0: ADC is set to this port, connects to the voltage divider constructed with the thermistor
PD2: Connects to TXD on HC-05 Bluetooth module to receive bluetooth commands from a mobile phone
PD3: Connects to RXD on CP210x to send serial to computer
*/

#define uart_print(s) uart_print_P(PSTR(s))
void uart_print_P(const char *str) {
	char c;
	while (1) {
		c = pgm_read_byte(str++);
		if (!c) break;
		uart_putchar(c);
	}
}

//initializes the adc module
void adc_init() {
	//AREF = AVcc
	DIDR0 = 0x13;
	ADMUX |= (1 << REFS0);
	//adc enable and prescaler of 128
	ADCSRA |= (1<<ADSC);
	ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

//reads adc, input is in form of 0x0Y, Y being the port number, typically use 0 or 1 to indicate PF0 or PF1
int16_t adc_read(uint8_t mux){
	uint8_t low;

	ADCSRA = (1<<ADEN) | ADC_PRESCALER;             // enable ADC
	ADCSRB = (1<<ADHSM) | (mux & 0x20);             // high speed mode
	ADMUX = aref | (mux & 0x1F);                    // configure mux input
	ADCSRA = (1<<ADEN) | ADC_PRESCALER | (1<<ADSC); // start the conversion
	while (ADCSRA & (1<<ADSC)) ;                    // wait for result
	low = ADCL;                                     // must read LSB first
	return (ADCH << 8) | low;                       // must read MSB only once!
}

void USART_Init( unsigned int ubrr) {
	/* Set baud rate */
	UBRR1H = (unsigned char)(ubrr>>8);
	UBRR1L = (unsigned char)ubrr;
	/* Enable receiver and transmitter and receive interrupt*/
	UCSR1B |= (1 << RXEN1) | (1 << TXEN1) | (1 << RXCIE1);
	/* Set frame format: 8data, 2stop bit */
	UCSR1C = (1<<USBS1)|(3<<UCSZ10);
}

//initializes the bluetooth
void uart_init(uint32_t baud){
	cli();
	UBRR1 = (F_CPU / 4 / baud - 1) / 2;
	UCSR1A = (1<<U2X1);
	UCSR1B = (1<<RXEN1) | (1<<TXEN1) | (1<<RXCIE1);
	UCSR1C = (1<<UCSZ11) | (1<<UCSZ10);
	tx_buffer_head = tx_buffer_tail = 0;
	rx_buffer_head = rx_buffer_tail = 0;
	sei();
}

// Return the number of bytes waiting in the receive buffer.
// Call this before uart_getchar() to check if it will need
// to wait for a byte to arrive.
uint8_t uart_available(void) {
	uint8_t head, tail;

	head = rx_buffer_head;
	tail = rx_buffer_tail;
	if (head >= tail) return head - tail;
	return RX_BUFFER_SIZE + head - tail;
}

//receives a character via serial from bluetooth
uint8_t UART_getc() {
	uint8_t c, i;

	while (rx_buffer_head == rx_buffer_tail) ; // wait for character
	i = rx_buffer_tail + 1;
	if (i >= RX_BUFFER_SIZE) i = 0;
	c = rx_buffer[i];
	rx_buffer_tail = i;
	return c;
}

//Sends a character via serial to computer
void uart_putchar(uint8_t c) {
	uint8_t i;

	i = tx_buffer_head + 1;
	if (i >= TX_BUFFER_SIZE) i = 0;
	while (tx_buffer_tail == i) ; // wait until space in buffer
	tx_buffer[i] = c;
	tx_buffer_head = i;
	UCSR1B = (1<<RXEN1) | (1<<TXEN1) | (1<<RXCIE1) | (1<<UDRIE1);
}

//Sends a string via serial to computer
void uart_putstring(char *s) {
	int i=0;
	while(s[i] != '\0') {
		uart_putchar(s[i]);
		i++;
	}
}

void delay_ms(uint8_t delay) {
	int x;
	for (x=0; x<delay; x++) {
		_delay_ms(1);
	}
}

void pulse(int dir, int duty) {
	//B0 = 1 is sends a cold pulse, B1 = 1 sends a hot pulse
	//dir == 0 indicates that the temp is too high
	if(dir==0) {
		PORTB |= (1<<PB0);
		PORTB &= ~(1<<PB1);
	}
	//dir == 1 indicates that the temp is too low
	else if (dir == 1) {
		PORTB &= ~(1<<PB0);
		PORTB |= (1<<PB1);
	}

	//delay sets the duration of the pulse to be proportionate to duty and the pw, pw is a defined value that is yet to be optimized
	delay_ms(pw * (duty / 100));
	//reset back to 0 after the pulse
	PORTB &= ~(1<<PB0);
	PORTB &= ~(1<<PB1);
	//second delay finishes
	delay_ms(pw * ((100 - duty)/100));
}

void holdtemp(char temp, char phase, char duty){
    uart_putstring("\n holdtemp start \n");
	//PULSE WIDTH * NUMBER OF PULSES = TOTAL PHASE TIME
	//pcount is the number of pulses that have to go through for one cycle to complete
	char pcount = (phase*1000/pw);

	char i;
	//phase timing and pulsing
	for(i=0; i<pcount; i++){ //goes through this function for pcount times, with each one having a pulse width of pw
		uint8_t ch = 0x00;
		uint16_t tempin = 128*adc_read(ch)/255;  //This gives the value of the voltage as 100*V

		//outputs the voltage reading via serial to computer
		uart_putstring("temp: ");
		uart_putchar(tempin);
		uart_putchar('\n');

		//each time it goes through a cycle the pulse function is run
		//depending on the input to pulse the function either will output a 1,0 0,1 or 0,0 to PB1 and PB0 which control the H bridge
		if(tempin >= temp + 5){ //if tempin is higher than the upper limit of acceptable temps, the 5 will affect the precision
			pulse(0, duty); // sending pulse a 0 indicates that the temp is high and to send a pulse to cold output
			uart_putchar('0'); //output character for debugging indicating which pulse was sent
		}
		else if (tempin <= temp - 5){// if tempin is below the range
			pulse(1, duty); // sending a pulse of 1 indiates that the temp is low and to send a pulse to the hot output
			uart_putchar('1'); //output character for debugging
		}
		else{ // else it is in the allowed range
			pulse(2, duty); // sending a 2 to pulse tells it not to send a pulse to either hot or cold
			uart_putchar('2'); //output character for debugging
		}
	}
}

//changetemp is called in the cycle function, its job is to adjust the temperature to the required range
//there is no time limit for this process because it is a do while loop
// unlike hold temp which runs for a set number of time it could run for forever if it never reaches the required temperature
void changetemp(char temp){
    uart_putstring("\n changetemp start \n");
    uint16_t tempin;
    do{
        char duty = 100; //indicates the percent uptime of the output during a pulse, 100 = 100%
        uint8_t ch = 0x00; // controls which port the ADC read from
        tempin = 128*adc_read(ch)/255;  //This gives the value of the voltage as 100*V

        //outputs the voltage reading via serial to computer
		uart_putstring("temp: ");
		uart_putchar(tempin);
		uart_putchar('\n');

		//each time it goes through a cycle the pulse function is run
		//depending on the input to pulse the function either will output a 1,0 0,1 or 0,0 to PB1 and PB0 which control the H bridge
		if(tempin >= temp + 5){ //if tempin is higher than the upper limit of acceptable temps, the 5 will affect the precision
			pulse(0, duty); // sending pulse a 0 indicates that the temp is high and to send a pulse to cold output
			uart_putchar('0'); //output character for debugging indicating which pulse was sent
		}
		else if (tempin <= temp - 5){// if tempin is below the range
			pulse(1, duty); // sending a pulse of 1 indiates that the temp is low and to send a pulse to the hot output
			uart_putchar('1'); //output character for debugging
		}
		else{ // else it is in the allowed range
			pulse(2, duty); // sending a 2 to pulse tells it not to send a pulse to either hot or cold
			uart_putchar('2'); //output character for debugging
		}
    }while(tempin > temp + 5 | tempin < temp - 5); //Until tempin is within the required range it will continue to be changed
}

void cycle() {
	//This function starts from the ISR
	//Ports stuff is for LED's indicating current phase
	PORTB &= ~(1<<PB4);
	PORTB &= ~(1<<PB7);// Turning of phase LEDs to indicate restarting the cycle and going back to phase 1
	//phase one
	changetemp(temp1); //each phase is the same as three just with different target temperatures and phase times
	PORTB |= (1<<PB3);
	uart_putchar('i');
	holdtemp(temp1, phase1, 100);
	uart_putchar('\n');
	uart_putchar('k');
	//phase two
	changetemp(temp2);
	uart_putchar('i');
	PORTB |= (1<<PB4);
	holdtemp(temp2, phase2, 100);
	uart_putchar('\n');
	uart_putchar('k');
	//phase three
	changetemp(temp3); //adjust to required temperature
	PORTB |= (1<<PB7); //LED indicating phase
	uart_putchar('i'); // Serial communication indicating that it has reached the required temperature and is now holding
	holdtemp(temp3, phase3, 100); // Holds Temperature for the amount of time indicated by phase variable
	uart_putchar('\n');
	uart_putchar('k'); // Sends a k via serial to indicate that it has finished the phase
}

ISR(USART1_UDRE_vect) {
	uint8_t i;

	if (tx_buffer_head == tx_buffer_tail) {
		// buffer is empty, disable transmit interrupt
		UCSR1B = (1<<RXEN1) | (1<<TXEN1) | (1<<RXCIE1);
	} else {
		i = tx_buffer_tail + 1;
		if (i >= TX_BUFFER_SIZE) i = 0;
		UDR1 = tx_buffer[i];
		tx_buffer_tail = i;
	}
}
//Interrupt upon receiving communication from the bluetooth module
ISR(USART1_RX_vect) {
	uint8_t c, i;

	c = UDR1;
	i = rx_buffer_head + 1;
	if (i >= RX_BUFFER_SIZE) i = 0;
	if (i != rx_buffer_tail) {
		rx_buffer[i] = c;
		rx_buffer_head = i;
	}
}

int main(void) {
	DDRB = 0b11111111;
	DDRF = 0b00000000;
	uart_init(BAUD_RATE);
	//USART_Init(USART1UBRR);
	adc_init();
	sei();
	_delay_ms(100);
	uart_putchar('i');

	while(1) {
		char c;
		c = UART_getc();
		if(c == 's'){
            uart_putstring("\n start");
            int x;
            for(x = 0; x < 10; x ++){
                cycle();
            }
            uart_putstring("\n \n Finished \n \n");
        }

		//as long as the device isn't getting a signal, keep the pulsing off
        PORTB &= ~(1<<PB0);
        PORTB &= ~(1<<PB1);
	}
	return 0;
}

/*OLD ISR FUNCTION, here for reference
ISR(USART1_RX_vect) {	//Interrupt triggered when data is received over USART1 (Bluetooth, from phone)
	uint8_t data_in;
	data_in = UDR1;
	USART_Transmit(data_in);
	char x;
	for(x=0; x<30; x++){//goes through 30 cycles, this will probably become a variable so that the
		uart_putchar('a');
		cycle();
	}
	uart_putstring('finished');
}*/
