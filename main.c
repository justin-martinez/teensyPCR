/*
 * Git_DNA.c
 *
 * Created: 7/9/2015 4:12:17 PM
 *  Author: martijus
 Editing the group code to see if we can optimize. 
 */ 
/*
	PCR: 
	The polymerase chain reaction (PCR) is a technology in molecular biology used to amplify a single copy 
	or a few copies of a piece of DNA across several orders of magnitude, generating thousands to millions 
	of copies of a particular DNA sequence.The method relies on thermal cycling, consisting of cycles of 
	repeated heating and cooling of the reaction for DNA melting and enzymatic replication of the DNA. 
	Primers (short DNA fragments) containing sequences complementary to the target region along with a DNA 
	polymerase, which the method is named after, are key components to enable selective and repeated 
	amplification. As PCR progresses, the DNA generated is itself used as a template for replication, 
	setting in motion a chain reaction in which the DNA template is exponentially amplified. PCR can be 
	extensively modified to perform a wide array of genetic manipulations.
	
	Uses:
	These include DNA cloning for sequencing, DNA-based phylogeny, or functional analysis of genes; the 
	diagnosis of hereditary diseases; the identification of genetic fingerprints (used in forensic sciences 
	and paternity testing); and the detection and diagnosis of infectious diseases.
	
	Steps:
	Initialization step: 
	(Only required for DNA polymerases that require
	heat activation by hot-start PCR): This step consists of heating
	the reaction to a temperature of 94–96 °C (or 98 °C if extremely thermostable
	polymerases are used), which is held for 1–9 minutes.
	
	Denaturation step: 
	This step is the first regular cycling event and
	consists of heating the reaction to 94–98 °C for 20–30 seconds. It causes
	DNA melting of the DNA template by disrupting the hydrogen bonds between
	complementary bases, yielding single-stranded DNA molecules.
	
	Annealing step: 
	The reaction temperature is lowered to 50–65 °C for 20–40
	seconds allowing annealing of the primers to the single-stranded DNA template.
	This temperature must be low enough to allow for hybridization of the primer to
	the strand, but high enough for the hybridization to be specific, i.e., the
	primer should only bind to a perfectly complementary part of the template. If
	the temperature is too low, the primer could bind imperfectly. If it is too high,
	the primer might not bind. Typically the annealing temperature is about 3–5 °C
	below the Tm of the primers used. Stable DNA–DNA hydrogen bonds are only formed
	when the primer sequence very closely matches the template sequence. The
	polymerase binds to the primer-template hybrid and begins DNA formation.
	
	Extension/elongation step: 
	The temperature at this step depends on the
	DNA polymerase used; Taq polymerase has its optimum activity temperature at
	75–80 °C, and commonly a temperature of 72 °C is used with this enzyme.
	At this step the DNA polymerase synthesizes a new DNA strand complementary
	to the DNA template strand by adding dNTPs that are complementary to the
	template in 5' to 3' direction, condensing the 5'-phosphate group of the dNTPs
	with the 3'-hydroxyl group at the end of the nascent (extending) DNA strand.
	The extension time depends both on the DNA polymerase used and on the length
	of the DNA fragment to amplify. As a rule-of-thumb, at its optimum temperature,
	the DNA polymerase polymerizes a thousand bases per minute. Under optimum
	conditions, i.e., if there are no limitations due to limiting substrates or
	reagents, at each extension step, the amount of DNA target is doubled, leading
	to exponential (geometric) amplification of the specific DNA fragment.
	
	Final elongation: 
	This single step is occasionally performed at a temperature of 70–74 °C (this 
	is the temperature needed for optimal activity for most polymerases used in PCR) 
	for 5–15 minutes after the last PCR cycle to ensure that any remaining 
	single-stranded DNA is fully extended.
	
	Final hold: 
	This step at 4–15 °C for an indefinite time may be employed for short-term storage 
	of the reaction.
 */ 
// Setting up timer
#define F_CPU           8000000UL	//Defining CPU speed for _ms_delay function (8Mhz clock)
#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n)) // Function to set ATMEGA32U4 clock speed
#define CPU_8MHz       0x01


#define ADC_PRESCALER	128
#define BAUD_RATE 38400
// These buffers may be any size from 2 to 256 bytes.
#define RX_BUFFER_SIZE 64
#define TX_BUFFER_SIZE 40
#define uart_print(s) uart_print_P(PSTR(s))
#define FAN_ON		(PINF |= (1<<4))
#define FAN_OFF		(PINF &= ~(1<<4))
#define pw 1000 // Defines Pulse wave time in ms, currently set to 1s for testing, could reduce for increased accuracy

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <avr/cpufunc.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

//******************START****************
//**********  Global Variables **********
//***************************************
//
static uint8_t aref = (1<<REFS0); // default to AREF = Vcc
static volatile uint8_t tx_buffer[TX_BUFFER_SIZE];
static volatile uint8_t tx_buffer_head;
static volatile uint8_t tx_buffer_tail;
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t rx_buffer_head;
static volatile uint8_t rx_buffer_tail;

//********** PCR Steps Time & Temperature

// Initialization step:
uint8_t init_step_time;
uint8_t init_step_temp;

//Denaturation step:
uint8_t Den_step_time;
uint8_t Den_step_temp;

// Annealing step:
uint8_t ann_step_time;
uint8_t ann_step_temp;

//Extension/elongation step:
uint8_t ext_step_time;
uint8_t ext_step_temp;

//Final elongation step:
uint8_t f_elon_step_time;
uint8_t f_elon_step_temp;

//Final hold:
uint8_t f_hold_step_time;
uint8_t f_hold_step_temp;

//temp variables
char temp1;//100:1 ratio of this number to voltage
char temp2;//max is 256 = 2.56 volts
char temp3;
//timing
char phase1 = 10;
char phase2 = 10;
char phase3 = 10;

//******************END******************
//**********  Global Variables **********
//***************************************


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


//USART1 Data Register Empty
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
//USART1 Rx Complete
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
//**************START**************
//**********  Functions  **********
//*********************************
void uart_print_P(const char*);
void adc_init();
int16_t adc_read(uint8_t);
void USART_Init( unsigned int);
void uart_init(uint32_t);
uint8_t uart_available(void);
uint8_t UART_getc();
void uart_putchar(uint8_t);
void uart_putstring(char*);
void delay_ms(uint16_t);
void delay_us(uint16_t);
void pulse(int, int);
void holdtemp(char, char, char);
void changetemp(char);
void cycle();
void Run_PCR (void);
void PCR_init(void);
void Counter_init (void);
//***************END***************
//**********  Functions  **********
//*********************************


int main(void) {
	CPU_PRESCALE(CPU_8MHz); // Set CPU clock speed
	DDRB = 0b11111111;
	DDRF = 0b00000000;
	uart_init(BAUD_RATE);
	//USART_Init(USART1UBRR);
	adc_init();
	sei();
	delay_ms(100);
	uart_putchar('i');
	int idle = 99;					//ASCII zero for no user input. This is the value designated for when user is idle
	int command = idle;				// input global variable user input into switch. Switch cannot take variable directly.
	
		while(1)
		{
			switch(command)
			{
				case(0):		// Initializes all Step parameters for temperature and time
				{
					PCR_init();
					break;
				}
			
				case(1):				// Runs the PCR. User can interrupt and cancel process from phone
				{
					Run_PCR();
					command = idle;	
					break;
				}
			
				default:							// If not command is sent the PCR will enter power saving "idle mode" see ATMEGA32U4 datasheet p.44
				{
					while (command != '0' |command != '1'){
					
						// standby Mode Control Register default is off 0000 0000
						SMCR = 00000110; // turn on power saving
						command = UART_getc();
						
						// User sends a "?" to initiate communication firmware sends "t" to confirm ready to receive.
						if(command == '?'){
								uart_putchar('t');
								command = idle;	
						}
					}
					SMCR = 00000000;
				}
			}
		}
	return 0;
}



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

void delay_ms(uint16_t delay) {
	delay = delay * 5 / 7;
	int x;
	for (x=0; x<delay; x++) {
		_delay_ms(1);
	}
}

void delay_us(uint16_t delay) {
	int x;
	for (x=0; x<delay; x++) {
		_delay_us(1);
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

	/*
	delay sets the duration of the pulse to be proportionate 
	to duty and the pw, pw is a defined value that is yet to be optimized
	*/
	delay_us(pw * (duty / 100));
	//reset back to 0 after the pulse
	PORTB &= ~(1<<PB0);
	PORTB &= ~(1<<PB1);
	//second delay finishes
	delay_us(pw * ((100 - duty)/100));
}

void holdtemp(char temp, char phase, char duty){
	uart_putchar('h');
	//PULSE WIDTH * NUMBER OF PULSES = TOTAL PHASE TIME
	//pcount is the number of pulses that have to go through for one cycle to complete
	char pcount = (phase*1000000/pw);

	char i;
	//phase timing and pulsing
	for(i=0; i<pcount; i++){ //goes through this function for pcount times, with each one having a pulse width of pw
		uint8_t ch = 0x00;
		uint16_t tempin = 128*adc_read(ch)/255;  //This gives the value of the voltage as 100*V

		//outputs the voltage reading via serial to computer
		//uart_putstring("temp: ");
		//uart_putchar(tempin);
		//uart_putchar('\n');

		//each time it goes through a cycle the pulse function is run
		//depending on the input to pulse the function either will output a 1,0 0,1 or 0,0 to PB1 and PB0 which control the H bridge
		if(tempin >= temp + 5){ //if tempin is higher than the upper limit of acceptable temps, the 5 will affect the precision
			pulse(0, duty); // sending pulse a 0 indicates that the temp is high and to send a pulse to cold output
			//uart_putchar('0'); //output character for debugging indicating which pulse was sent
		}
		else if (tempin <= temp - 5){// if tempin is below the range
			pulse(1, duty); // sending a pulse of 1 indiates that the temp is low and to send a pulse to the hot output
			//uart_putchar('1'); //output character for debugging
		}
		else{ // else it is in the allowed range
			pulse(2, duty); // sending a 2 to pulse tells it not to send a pulse to either hot or cold
			//uart_putchar('2'); //output character for debugging
		}
	}
}

//changetemp is called in the cycle function, its job is to adjust the temperature to the required range
//there is no time limit for this process because it is a do while loop
// unlike hold temp which runs for a set number of time it could run for forever if it never reaches the required temperature
void changetemp(char temp){
	uart_putchar('c');
	uint16_t tempin;
	do{
		char duty = 100; //indicates the percent uptime of the output during a pulse, 100 = 100%
		uint8_t ch = 0x00; // controls which port the ADC read from
		tempin = 128*adc_read(ch)/255;  //This gives the value of the voltage as 100*V

		//outputs the voltage reading via serial to computer
		//uart_putstring("temp: ");
		//uart_putchar(tempin);
		//uart_putchar('\n');

		//each time it goes through a cycle the pulse function is run
		//depending on the input to pulse the function either will output a 1,0 0,1 or 0,0 to PB1 and PB0 which control the H bridge
		if(tempin >= temp + 1){ //if tempin is higher than the upper limit of acceptable temps, the 5 will affect the precision
			pulse(0, duty); // sending pulse a 0 indicates that the temp is high and to send a pulse to cold output
			//uart_putchar('0'); //output character for debugging indicating which pulse was sent
		}
		else if (tempin <= temp - 1){// if tempin is below the range
			pulse(1, duty); // sending a pulse of 1 indicates that the temp is low and to send a pulse to the hot output
			//uart_putchar('1'); //output character for debugging
		}
		else{ // else it is in the allowed range
			pulse(2, duty); // sending a 2 to pulse tells it not to send a pulse to either hot or cold
			//uart_putchar('2'); //output character for debugging
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
	//uart_putchar('i');
	holdtemp(temp1, phase1, 100);
	//uart_putchar('\n');
	//uart_putchar('k');
	//phase two
	changetemp(temp2);
	//uart_putchar('i');
	PORTB |= (1<<PB4);
	holdtemp(temp2, phase2, 100);
	//uart_putchar('\n');
	//uart_putchar('k');
	//phase three
	changetemp(temp3); //adjust to required temperature
	PORTB |= (1<<PB7); //LED indicating phase
	//uart_putchar('i'); // Serial communication indicating that it has reached the required temperature and is now holding
	holdtemp(temp3, phase3, 100); // Holds Temperature for the amount of time indicated by phase variable
	//uart_putchar('\n');
	//uart_putchar('k'); // Sends a k via serial to indicate that it has finished the phase
}

void PCR_init(void){
	unsigned char pcr_set[12];
	uint8_t confirm = 0;
	int i;
	
	// while loop will check with user to verify that input are correct.
	while(confirm != 1){
		//Getting user settings from bluetooth (12 values: 6 temp, 6 time)
		for(i=0; i<11;i++){
			pcr_set[i]= UART_getc();
		}
		
		//Confirming user settings
		for(i=0; i<11;i++){
			//Flag to user: "0" message indicating sending variable.
			uart_putchar('0');
			// Sending data back one variable at a time.
			uart_putchar(pcr_set[i]);
		}
		
		//********** PCR Steps Time & Temperature

		// Initialization step:
		uint8_t init_step_time = pcr_set[0];
		uint8_t init_step_temp = pcr_set[1];

		//Denaturation step:
		uint8_t Den_step_time = pcr_set[2];
		uint8_t Den_step_temp = pcr_set[3];

		// Annealing step:
		uint8_t ann_step_time = pcr_set[4];
		uint8_t ann_step_temp = pcr_set[5];

		//Extension/elongation step:
		uint8_t ext_step_time = pcr_set[6];
		uint8_t ext_step_temp = pcr_set[7];

		//Final elongation step:
		uint8_t f_elon_step_time = pcr_set[8];
		uint8_t f_elon_step_temp = pcr_set[9];

		//Final hold:
		uint8_t f_hold_step_time = pcr_set[10];
		uint8_t f_hold_step_temp = pcr_set[11];
		
		confirm = UART_getc();
		if(confirm == '1'){
			break; 
		}
	}
}
void Run_PCR (void)
{
	char x;
	for(x = 0; x < 30; x ++){
		cycle();
		uart_putchar(x);
	}
	uart_putstring("\n \n Finished \n \n");
}

void Counter_init (void)	//initializes hour counter with 1024/8MHz prescaler
{
	/* 
	Timer/Counter1 Control Register A = TCCR1A 
	Timer/Counter1 Control Register B = TCCR1B
	Timer/Counter1 Interrupt Mask Register = TIMSK1
	*/
	TIMSK0 = ((1<<OCIE0A) | (1<<OCIE0B));  // Enable Interrupt TimerCounter0 Compare Match A & B
	  

	TCCR1A = (1<<WGM01);	//Setting TCCR1A Mode to CTC. TCCR1A = 0000 0000
	
	/*  Bit 2 (CS12 = Clock Select selects the clock source to be used by the Timer/Counter)
		Bit 3 CTC clear on compare match (WGM12)
		Bit 7 – FOC1B: Force Output Compare for Channel B
		The FOCnA/FOCnB/FOCnC bits are only active when the WGMn3:0 bits specifies a non-PWM
		mode. When writing a logical one to the FOCnA/FOCnB/FOCnC bit, an immediate compare
		match is forced on the waveform generation unit.
		
	*/
	TCCR1B |= ((1<<CS12)(1<<WGM12)); //Setting TCCR1B initial value to 256 prescaler (ATMEGA32U4 datasheet p.133). TCCR1A = 0000 0100
	
	/* Bit 0 – TOIE1: Timer/Counter1, Overflow Interrupt Enable
	When this bit is written to one, and the I-flag in the Status Register is set (interrupts globally
	enabled), the Timer/Counter1 Overflow interrupt is enabled. */
	TIMSK1 |= (1<<TOIE1);
}
