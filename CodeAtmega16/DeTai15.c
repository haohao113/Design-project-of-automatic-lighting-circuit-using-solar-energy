#include <mega16.h>
#include <delay.h>
#include <stdio.h>
#include <alcd.h>

#define F_CPU 8000000UL
#define BAUD 9600
#define UBRR_VALUE ((F_CPU / (BAUD * 16UL)) - 1)

#define PIR_SENSOR PINC .0

#define LIGHT_SENSOR_PIN 0

#define BUTTON1_PIN PIND .2
#define BUTTON2_PIN PIND .3
#define BUTTON3_PIN PIND .4
#define BUTTON4_PIN PIND .5

#define LED_PIN PORTB .3

#define ADC_VREF_TYPE ((0 << REFS1) | (1 << REFS0) | (0 << ADLAR))

unsigned int light_threshold_day = 512;
unsigned int light_threshold_night = 512;

unsigned char light_threshold = 50;
unsigned char mode = 0; // 1 for mode 1, 2 for mode 2
unsigned char mode3 = 0; // 1 for mode 1, 2 for mode 2
unsigned char chedo = 0;
unsigned int dosang, dosang1;
unsigned char song = 1;       // Start with song 1
unsigned char prev_chedo = 0; // Keep track of the previous mode
char buffer[16];

unsigned int light_value;
int delay_counter = 0;

void UART_init(unsigned int ubrr)
{
    UBRRH = (unsigned char)(ubrr >> 8);
    UBRRL = (unsigned char)ubrr;
    UCSRB = (1 << RXEN) | (1 << TXEN);
    UCSRC = (1 << URSEL) | (3 << UCSZ0);
}

void ADC_init()
{
    // Ch?n AVCC l�m di?n �p tham chi?u, kh�ng k�ch ho?t ch?c nang left adjust
    ADMUX = (1 << REFS0);

    // K�ch ho?t ADC, kh�ng ch?n ch? d? chuy?n d?i li�n t?c
    ADCSRA = (1 << ADEN);

    // Ch?n ch? d? chia t? l? 64 (8MHz / 64 = 125kHz)
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1);
}

unsigned int read_adc(unsigned char adc_input)
{
    // Switch ADC channel
    ADMUX = (ADMUX & 0xF8) | (adc_input & 0x07);
    // Wait for channel switch to take effect
    delay_us(10);
    // Start a new AD conversion
    ADCSRA |= (1 << ADSC);
    // Wait for the conversion to complete
    while ((ADCSRA & (1 << ADIF)) == 0)
        ;
    // Clear the ADIF bit
    ADCSRA |= (1 << ADIF);
    return ADCW;
}

void UART_transmit(unsigned char data)
{
    while (!(UCSRA & (1 << UDRE)))
        ;
    UDR = data;
}

void sendCommand(unsigned char command, unsigned int argument)
{
    UART_transmit(0x7E);
    UART_transmit(0xFF);
    UART_transmit(0x06);
    UART_transmit(command);
    UART_transmit(0x00);
    UART_transmit((unsigned char)(argument >> 8));
    UART_transmit((unsigned char)argument);
    UART_transmit(0xEF);
    delay_ms(10);
}
unsigned int map(unsigned int x, unsigned int in_min, unsigned int in_max, unsigned int out_min, unsigned int out_max)
{
    return ((long)(x - in_min) * (long)(out_max - out_min)) / (in_max - in_min) + out_min;
}
void main(void)
{

    DDRC = 0 << DDC0;
    DDRB = 1 << DDB3;
    PORTB .3 = 1;
    DDRD = (0 << DDD2) | (0 << DDD3) | (0 << DDD4) | (0 << DDD5);

    UART_init(UBRR_VALUE);
    ADC_init();
    lcd_init(16);

    //    lcd_gotoxy(0,0);
    //    lcd_puts("Khoi Dong");
    //
    //    sendCommand(0x03, 5);
    //    delay_ms(6000);

    while (1)
    {
        if (!(BUTTON2_PIN))
        {
            chedo = (chedo + 1) % 3;
        }
        while (!(BUTTON2_PIN))
            ;
        switch (chedo)
        {
        case 0:
            light_value = read_adc(0);
            dosang = map(light_value, 0, 1023, 0, 100); // Map to 8-bit range
            dosang = 100 - dosang;
            lcd_clear();
            sprintf(buffer, "Light:%d", dosang);
            lcd_gotoxy(0, 0);
            lcd_puts(buffer);
            if (!(BUTTON1_PIN))
            {
                mode = (mode + 1) % 2;
            }
            while (!(BUTTON1_PIN))
                ;
            if (mode == 0)
            {
                lcd_gotoxy(9, 0);
                lcd_puts("AUTO");
            }
            else
            {
                lcd_gotoxy(9, 0);
                lcd_puts("MANNUAL");
            }
            if (PIR_SENSOR && ((mode == 0 && dosang <= light_threshold) || mode == 1))
            {
                //                      if(delay_counter == 0) { // Only trigger the action once
                LED_PIN = 0;
                sendCommand(0x03, 6);
                delay_counter = 150; // Set the counter for 15 seconds delay
                                     //                      }
                lcd_gotoxy(0, 1);
                lcd_puts("Motion Detected");
            }
            else
            {
                if (delay_counter == 0)
                { // Only reset the LED and LCD when not in delay
                    LED_PIN = 1;
                }
                lcd_gotoxy(0, 1);
                lcd_puts("No Motion       ");
            }
            break;
        case 1:
            // Adjust light_threshold with BUTTON3 and BUTTON4
            if (!(BUTTON3_PIN))
            {
                light_threshold++;
                delay_ms(200);
            }
            if (!(BUTTON4_PIN))
            {
                light_threshold--;
                if (light_threshold < 0)
                    light_threshold = 0; // Prevent negative values
                delay_ms(200);
            }

            // Display current light_threshold on LCD
            lcd_clear();
            sprintf(buffer, "Gioi Han A/S: %d", light_threshold);
            lcd_gotoxy(0, 0);
            lcd_puts(buffer);
            break;
        case 2:
            if (!(BUTTON1_PIN))
            {
                mode3 = (mode3 + 1) % 2;
            }
            while (!(BUTTON1_PIN))
                ;
            if (mode3 == 0)
            {   
                lcd_clear();
                lcd_gotoxy(2, 1);
                lcd_puts("Chua Mo Nhac");
            }
            else
            {
                if (!(BUTTON3_PIN))
                {
                    song = (song % 4) + 1; // Cycle through songs 1-10
                }
                while (!(BUTTON3_PIN))
                    ;

                if (!(BUTTON4_PIN))
                {
                    song = ((song - 2 + 4) % 4) + 1; // Cycle through songs 10-1
                }
                while (!(BUTTON4_PIN))
                    ; // Wait for button release

                sendCommand(0x03, song); // Send command to play the selected song
                lcd_clear();
                sprintf(buffer, "Playing song: %d", song);
                lcd_gotoxy(0, 0);
                lcd_puts(buffer);
                lcd_gotoxy(3, 1);
                lcd_puts("MO NHAC");
            }
            break;
        }
        if (delay_counter > 0)
        {
            delay_counter--; // Decrement the delay counter if it's greater than 0
        }
        delay_ms(100);
    }
}

