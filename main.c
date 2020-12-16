#define F_CPU 1000000 // Clock Speed

#include <util/delay.h>

#include <avr/io.h>

#define BAUD 19200
#define MYUBRR F_CPU/16/BAUD-1

void USART_Init( unsigned int ubrr)
{
  /*Set baud rate */
  UBRR0H = (unsigned char)(ubrr>>8);
  UBRR0L = (unsigned char)ubrr;
  /* Enable receiver and transmitter */
  UCSR0B = (1<<RXEN0)|(1<<TXEN0);
  /* Set frame format: 8data, 1stop bit */
  UCSR0C = (3<<UCSZ00);
}

void USART_Transmit( unsigned char data )
{
  /* Wait for empty transmit buffer */
  while ( !( UCSR0A & (1<<UDRE0)) )
  ;

  /* Put data into buffer, sends the data */
  UDR0 = data;
}

static void
delay_1s(void)
{
  uint8_t i;

  for (i = 0; i < 100; i++)
    _delay_ms(10);
}

static void
delay_100ms(void)
{
  //uint8_t i;

  //for (i = 0; i < 10; i++)
    _delay_ms(100);
}

static void
delay_25ms(void)
{
  uint8_t i;

  for (i = 0; i < 5; i++)
    _delay_ms(5);
}

static void
delay_800us(void)
{
  uint8_t i;

  //for (i = 0; i < 80; i++)
    _delay_us(800);
}

static void
delay_200us(void)
{
  uint8_t i;

  for (i = 0; i < 20; i++)
    _delay_us(10);
}

//Connect LN1 or LN2 to output LN
//0 - connect LN1
//1 - connect LN2
//2 - unconnect 
//3 - all connected
void ln_sel( unsigned char num ) {
  if ( num == 1 ) {
    //LN+ switch to LN1+ or LN2+
    PORTB |= (1 << DDB0);
    //LN1- switch to -LN
    PORTB &= ~(1 << DDB1);
    //LN2- switch to -LN
    PORTB |= (1 << DDB2);
    //USART_Transmit(0x01);
    _delay_ms(100);
  }
  else if ( num == 2 ) {
    //Switch LN1- and LN2- to internal load
    //LN+ switch to LN1+ or LN2+
    PORTB &= ~(1 << DDB0);
    //PORTB |= (1 << DDB0);
    //LN1- switch to -LN
    PORTB &= ~(1 << DDB1);
    //LN2- switch to -LN
    PORTB &= ~(1 << DDB2);
    _delay_ms(100);
    //USART_Transmit(0x02);
  }
  else if ( num == 0 ){
    //LN+ switch to LN1+ or LN2+
    PORTB &= ~(1 << DDB0);
    //LN1- switch to -LN
    PORTB |= (1 << DDB1);
    //LN2- switch to -LN
    PORTB &= ~(1 << DDB2);
    _delay_ms(100);
    //USART_Transmit(0x00);
  }
  else if ( num == 3 ){
    //Switch LN1- to LN2-
    //LN+ switch to LN1+ or LN2+
    //PORTB &= ~(1 << DDB0);
    PORTB |= (1 << DDB0);
    //LN1- switch to -LN
    PORTB |= (1 << DDB1);
    //LN2- switch to -LN
    PORTB |= (1 << DDB2);
    _delay_ms(100);
    //USART_Transmit(0x02);
  }
}

//void WDT_Prescaler_Change(void)
//{
//  __disable_interrupt();
//  __watchdog_reset();
//  /* Start timed equence */
//  WDTCSR |= (1<<WDCE) | (1<<WDE);
//  /* Set new prescaler(time-out) value = 64K cycles (~0.5
//  s) */
//  WDTCSR = (1<<WDE) | (1<<WDP2) | (1<<WDP0);
//  __enable_interrupt();
//}

enum States
{
    ST_IDLE,
    ST_CALL_LN1,
    ST_CALL_LN2,
} state;  

// Основная функция программы.
void main(void) {
  unsigned char call_cnt_len = 0;

  unsigned char ln1_ncall;
  unsigned char ln2_ncall;

  unsigned char video_k2;
  unsigned char video_k3;
  unsigned char video_k4;

  unsigned short call_period = 0;
  unsigned short call_ln1_pulse_cnt = 0;
  unsigned short call_ln2_pulse_cnt = 0;

  USART_Init(MYUBRR);

  //Uart rx/tx set input
  DDRD  &= ~(1 << DDD0); //UART rx
  DDRD  |= (1 << DDD1); //UART tx

  DDRC |= (1 << DDC0); //Vi0 switch enable
  DDRC |= (1 << DDC1); //Vi1 switch enable
  DDRC |= (1 << DDC2); //Vi2 switch enable
  DDRC |= (1 << DDC3); //Vi3 switch enable

  //Turn off all video switchs by default
  PORTC &= ~(1 << DDC0);//
  PORTC &= ~(1 << DDC1);//
  PORTC &= ~(1 << DDC2);//
  PORTC &= ~(1 << DDC3);//

  DDRC |= (1 << DDC5); //RS485 tx/rx mode control
  PORTC |= (1 << DDC5);//RS485 tx mode

  //LN+ switch to LN1+ or LN2+
  DDRB |= (1 << DDB0);
  //LN1- switch to -LN
  DDRB |= (1 << DDB1);
  //LN2- switch to -LN
  DDRB |= (1 << DDB2);

  //IN1- input
  DDRD &= ~(1 << DDD2);
  //IN2- input
  DDRD &= ~(1 << DDD3);
  //When there is signal on LN1 or LN2 it's goes to low, otherwise high
  PORTD |= (1 << DDD2);//
  PORTD |= (1 << DDD3);//

  //Jumpers K2, k3, k4
  DDRD &= ~(1 << DDD4);
  DDRD &= ~(1 << DDD5);
  DDRD &= ~(1 << DDD6);
  //When jumper is switch circuit there is ZERO else ONE
  PORTD |= (1 << DDD4);//
  PORTD |= (1 << DDD5);//
  PORTD |= (1 << DDD6);//

  //In mode 2 seems to can read LN1 and LN2 line
  //ln_sel(3); //LN2 have no load, no calling
  //ln_sel(2);
  //ln_sel(1);
  ln_sel(2);

  //Jumpers k4, k3, k2 to control video in ports:
  //k3,k2
  //000 - no video in at all, switch only LN1, Ln2 lines
  //001 - only vi1 preset
  //010 - only vi2 preset
  //011 - vi1 and vi2 input
  //where 0 jumper unset and 1 jumper set
  //video_k4k3k2 = (~(((PIND & (1 << DDD4))|(PIND & (1 << DDD5))| (PIND & (1 << DDD6)))>>4)) & 0x7;
  video_k2 = (~((PIND & (1 << DDD4)) >> DDD4)) & 0x1;
  video_k3 = (~((PIND & (1 << DDD5)) >> DDD5)) & 0x1;
  video_k4 = (~((PIND & (1 << DDD6)) >> DDD6)) & 0x1;

  delay_1s();

  //USART_Transmit(video_k2);
  //delay_1s();
  //USART_Transmit(video_k3);
  //delay_1s();
  //USART_Transmit(video_k4);

  //PORTC |= (1 << DDC0);//Turn on video Vi1
  //PORTC |= (1 << DDC1);//Turn on video Vi2

  //for(;;) {
  //  delay_800us();
  //}

  for(;;) { 
    delay_800us();

    //Read state LN1 and LN2 line
    ln1_ncall = PIND & (1 << DDD2);
    ln2_ncall = PIND & (1 << DDD3);

    //Wait call pulse and count it.
    //There is 2500 800us tick in 2 second. There is meandr 500ms length and 1600us period when there is calling.
    if ( ln1_ncall == 0 ) 
      call_ln1_pulse_cnt = call_ln1_pulse_cnt + 1;
    if ( ln2_ncall == 0 ) 
      call_ln2_pulse_cnt = call_ln2_pulse_cnt + 1;

    //There is 2500 800us tick in 2 second. There is meandr 500ms length and 1600us period when there is calling.
    if ( call_period == 2499 ) {

      call_period = 0;

      call_ln1_pulse_cnt = 0;
      call_ln2_pulse_cnt = 0;

    } else {
      call_period = call_period + 1;
    }

    if ( call_period == 2499  ) {

      video_k2 = (~((PIND & (1 << DDD4)) >> DDD4)) & 0x1;
      video_k3 = (~((PIND & (1 << DDD5)) >> DDD5)) & 0x1;
      video_k4 = (~((PIND & (1 << DDD6)) >> DDD6)) & 0x1;

      switch(state) {
        case ST_IDLE:
          //USART_Transmit(0x0);
          //DVR use first channel
          if ( video_k2 ) {
            PORTC |= (1 << DDC0);//Turn on video Vi1
          }

          if ( call_ln1_pulse_cnt > 100 ) {
            state = ST_CALL_LN1;
            //Make switching to LN1 line
            ln_sel(0);
            //USART_Transmit(0x1);
          } else if ( call_ln2_pulse_cnt > 100  ) {
            state = ST_CALL_LN2;
            //Make switching to LN2 line
            ln_sel(1);
            if ( video_k3 ) {
              PORTC &= ~(1 << DDC0);//Turn off video Vi1
              PORTC |= (1 << DDC1); //Turn on video Vi2
            }
            USART_Transmit(video_k3);
          }
          break;
        case ST_CALL_LN1:
          //USART_Transmit(0x3);
          //On real line after call LN do not goes low, always high.
          //So this condition do not work.
          if ( call_ln1_pulse_cnt == 0 ) {
          //call_cnt_len = call_cnt_len + 1;
          //Give 30 seconds for call
          //if ( call_cnt_len == 12 ) {
            call_cnt_len = 0;
            state = ST_IDLE;
            //Unconnect from line
            ln_sel(2);
            //PORTC &= ~(1 << DDC0);//
            //USART_Transmit(0x4);
          }
          break;
        case ST_CALL_LN2:
          //USART_Transmit(0x5);
          if ( call_ln2_pulse_cnt == 0 ) {
          //call_cnt_len = call_cnt_len + 1;
          //Give 30 seconds for call
          //if ( call_cnt_len == 12 ) {
            call_cnt_len = 0;
            state = ST_IDLE;
            //Unconnect from line
            ln_sel(2);
            PORTC &= ~(1 << DDC1);//
            //USART_Transmit(0x6);
          }
         break;
      }
    }
  }
}
