/*
 * ESE519_Lab4_Pong_Starter.c
 *
 * Author : Tarunyaa Sivakumar 
 */ 

#define F_CPU 16000000UL
#define BAUD_RATE 9600
#define BAUD_PRESCALER (((F_CPU / (BAUD_RATE * 16UL))) - 1)
#include <time.h>
#include <avr/io.h>
#include "../lib/ST7735.h"
#include "../lib/LCD_GFX.h"
#include <stdlib.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include<time.h>
#include <stdbool.h>
#include "../lib/uart.h"
/*--------------------Variables---------------------------*/
char String[25]; // Stores sequence of characters
float paddley = 0 ; // Current Paddle y Position in float
int paddleyInt = 75 ; // Current Paddle y Position in int
volatile int prevPaddleyInt = 0; // Previous Paddle y Position in int
volatile int CompPaddley = 25 ; // Current Computer Paddle y Position
volatile bool CompPaddleDirection = true ; // Current Computer Paddle y Position
volatile int inPositionX = 80; // Initial x Position of the Ball
volatile int inPositionY = 64; // Initial  y Position of the Ball
volatile int positionX = 80; // Current x Position of the Ball
volatile int positionY = 64; // Current y Position of the Ball
volatile int velocityX = 0 ; // x Velocity of Ball
volatile int velocityY = 0 ; // y Velocity of Ball
volatile int prevCompPoints;
volatile int compPoints = 0;
volatile char compPointsChar = 0;
volatile int prevUserPoints;
volatile int userPoints = 0;
volatile char userPointsChar = 0;
volatile int duty = 0;
bool gameOn = true;
volatile int number = 1;
volatile int prevnumber = 1;
volatile int userWins = 0;
volatile int compWins = 0;

/*-----------------------------------------------------------*/

void Initialize()
{
    cli();

    // Initializing output pins
    DDRD |= (1<<DDD4);// Set PD4 to the user LED (blue LED)
    DDRD |= (1<<DDD5);// Set PD5 to the comp LED (green LED)
    DDRD |= (1<<DDD3);// Set PD3 to the output pin (buzzer) (OC0B)
    PORTD &= ~(1<<PORTD5); // Switching off LEDs for now
    PORTD &= ~(1<<PORTD4); // Switching off LEDs for now

    // LCD Screen Setup
    lcd_init();

    // Set Screen
    LCD_setScreen(rgb565(255, 255, 255)); // Set to white
    sprintf(String, "Starting Round %u", number); // Message
    LCD_drawString(0, 64, String, rgb565(0, 0, 0), rgb565(255, 255, 255));
    _delay_ms(5000);

    LCD_drawBlock(0, 0, 180, 115,rgb565(0, 0, 0)); // Draw black block with white scoreboard
    sprintf(String, "R:%u", number);
    LCD_drawString(25, 120, String, rgb565(0, 0, 0), rgb565(255, 255, 255));
    sprintf(String, "U:%u", userPoints);
    LCD_drawString(65, 120, String, rgb565(0, 0, 0), rgb565(255, 255, 255));
    sprintf(String, "C:%u", compPoints);
    LCD_drawString(105, 120, String, rgb565(0, 0, 0), rgb565(255, 255, 255));

    // Creating User Paddle
    LCD_drawBlock(0, 50, 3, 75,rgb565(255, 255, 255));

    // Creating Computer Paddle
    LCD_drawBlock(154, 50, 157, 75,rgb565(255, 255, 255));

    // Initialising ball position
    positionX = inPositionX;
    positionY = inPositionY;

    srand(time(NULL));  ; // To generate different random number everytime
    velocityX = ((rand() % 6) + 5);
    velocityY = ((rand() % 6) + 5);

    // Timer2 Setup for Buzzer
    // Set Timer 2 clock to have prescaler of 64
    TCCR2B |= (1<<CS20);
    TCCR2B |= (1<<CS21);
    TCCR2B &= ~(1<<CS22);

    // Set Timer 0 to fast PWM for ADC
    TCCR2A |= (1<<WGM20);
    TCCR2A |= (1<<WGM21);
    TCCR2B |= (1<<WGM22);

    OCR2A = 100; // initial value
    duty = 5;
    OCR2B = (OCR0A * duty) / 100; // initial value

    // ADC Setup
    DDRC &= ~(1<<DDC0); // Set AD0 pin

    // Clear power reduction for ADC
    PRR &= ~(1<<PRADC);

    // Select Vref = AVcc
    ADMUX |= (1<<REFS0);
    ADMUX &= ~(1<<REFS1);

    // Set ADC clock to have prescaler of 128 (125kHz where ADC needs 50 - 200 kHz)
    ADCSRA |= (1<<ADPS0);
    ADCSRA |= (1<<ADPS1);
    ADCSRA |= (1<<ADPS2);

    // Select channel 0
    ADMUX &= ~(1<<MUX0);
    ADMUX &= ~(1<<MUX1);
    ADMUX &= ~(1<<MUX2);
    ADMUX &= ~(1<<MUX3);

    // Set to auto trigger
    ADCSRA |= (1<<ADATE);

    // Set to free running
    ADCSRB &= ~(1<<ADTS0);
    ADCSRB &= ~(1<<ADTS1);
    ADCSRB &= ~(1<<ADTS2);

    // Disable digital input buffer on ADC pin
    DIDR0 |= (1<<ADC0D);

    // Enable ADC
    ADCSRA |= (1<<ADEN);

    // Start conversion
    ADCSRA |= (1<<ADSC);

    sei();
}

void readJoystick(void) {
    prevPaddleyInt = paddleyInt;
    paddley = 115.0/1024.0 * ADC; // Converting ADC value to paddle y coordinate
    sprintf(String, "ADC: %u \n", ADC);
    UART_putstring(String);
    paddleyInt = paddley;
    if (paddleyInt < 25) {
        paddleyInt = 25;
    }

    // Removing and Replacing Previous User Paddle
    if (!(prevPaddleyInt == paddleyInt)) {
        LCD_drawBlock(0, prevPaddleyInt - 25, 3, prevPaddleyInt,rgb565(0, 0, 0));
        LCD_drawBlock(0, paddleyInt - 25, 3, paddleyInt,rgb565(255, 255, 255));
    }

    ADCSRA |= (1<<ADSC); // Restart conversion
}

void setCompPaddle(void) {
	
    // Ensuring that the paddle moves back and forth
    if ((CompPaddley + 5) > 115) {
        CompPaddleDirection = false;
    }

    if ((CompPaddley - 5) < 27) {
        CompPaddleDirection = true;
    }

    if(CompPaddleDirection) {
        CompPaddley = CompPaddley + 5;
    } else {
        CompPaddley = CompPaddley - 5;
    }

    LCD_drawBlock(154, 0, 157, 115,rgb565(0, 0, 0));
    LCD_drawBlock(154, CompPaddley - 25, 157, CompPaddley,rgb565(255, 255, 255));

}


void removeBall(void) {
    LCD_drawCircle(positionX, positionY, 3,rgb565(0, 0, 0));
}

void genRandomVelocity(void) {
    // Between -10 and 10 in increments of 2
    velocityX = (((rand() % 11) - 5))*2;
    velocityY = (((rand() % 11) - 5))*2;

    if(velocityX == 0) {
        genRandomVelocity();
    }
}

void isBallOutOfBounds(void) {

    // Check user paddle
    if ((velocityX <= 0) && (abs(velocityX)>=positionX - 6)) {
        velocityX = - velocityX;
        if (positionY <= paddleyInt && positionY >= (paddleyInt-25)) {
        } else {
            compPoints ++;
            PORTD |= (1<<PORTD4); // Switch on LED
            positionX = inPositionX;
            positionY = inPositionY;
            genRandomVelocity();
        }
        TCCR2A &= ~(1<<COM2B0); // Switch on buzzer
        TCCR2A |= (1<<COM2B1);
    }

    // Check bottom of screen
    if ((velocityY <= 0) && (abs(velocityY)>=positionY)) {
        positionY = 0;
        velocityY = - velocityY;
        TCCR2A &= ~(1<<COM2B0);
        TCCR2A |= (1<<COM2B1);
    }

    // Check computer paddle
    if ((velocityX >= 0) && (positionX>=(151-velocityX))) {
        velocityX = - velocityX;
        if (positionY <= CompPaddley && positionY >= (CompPaddley-25)) {
        } else {
            userPoints++;
            PORTD |= (1<<PORTD5); // Switch on LED
            positionX = inPositionX;
            positionY = inPositionY;
            genRandomVelocity();
        }
        TCCR2A &= ~(1<<COM2B0); // Switch on buzzer
        TCCR2A |= (1<<COM2B1);
    }

    // Check top of screen
    if ((velocityY >= 0) && (positionY>=(112-velocityY))) {
        positionY = 112;
        velocityY = - velocityY;
        TCCR2A &= ~(1<<COM2B0);
        TCCR2A |= (1<<COM2B1);
    }


}

void updateBall(void) {
    positionX += velocityX;
    positionY += velocityY;

    LCD_drawCircle(positionX, positionY, 3,rgb565(255, 255, 255));
}

void resetPeri(void) {
    // Resetting peripherals including LED and buzzer
    PORTD &= ~(1<<PORTD5);
    PORTD &= ~(1<<PORTD4);
    TCCR2A |= (1<<COM2B0);
    TCCR2A &= ~(1<<COM2B1);
}

void pointTallying() {
    compPointsChar = compPoints + '0';
    userPointsChar = userPoints + '0';

    if (!(prevnumber == number)) {
        sprintf(String, "R:%u", number);
        LCD_drawString(25, 120, String, rgb565(0, 0, 0), rgb565(255, 255, 255));
    }

    if (!(prevUserPoints == userPoints)) {
        sprintf(String, "U:%u", userPoints);
        LCD_drawString(65, 120, String, rgb565(0, 0, 0), rgb565(255, 255, 255));
    }

    if (!(prevCompPoints == compPoints)) {
        sprintf(String, "C:%u", compPoints);
        LCD_drawString(105, 120, String, rgb565(0, 0, 0), rgb565(255, 255, 255));
    }

    // Go to next round if user or computer scores 5 points
    if (compPoints >= 5 | userPoints >= 5) {
        resetPeri();
        if (compPoints >= 3) {
            compWins ++;
        } else {
            userWins ++;
        }
        _delay_ms(2000);
        number += 1;
        if (number == 4) { // 3 rounds in total, implementing best of 3 rounds
            LCD_setScreen(rgb565(255, 255, 255)); // Set to white
            if (userWins > compWins) {
                sprintf(String, "User Wins!");
                LCD_drawString(30, 64, String, rgb565(0, 0, 0), rgb565(255, 255, 255));
            } else if (userWins < compWins) {
                sprintf(String, "Computer Wins :(");
                LCD_drawString(20, 64, String, rgb565(0, 0, 0), rgb565(255, 255, 255));
            } else {
                sprintf(String, "Game Tied");
                LCD_drawString(30, 64, String, rgb565(0, 0, 0), rgb565(255, 255, 255));
            }
            _delay_ms(10000);
            gameOn = false;
        } else {
            prevUserPoints = 0;
            userPoints = 0;
            prevCompPoints = 0;
            compPoints = 0;
            Initialize();
        }
    }
}

int main(void)
{
    UART_init(BAUD_PRESCALER);
    Initialize();


    while(gameOn)
    {
        resetPeri();
        prevnumber = number;
        prevUserPoints = userPoints;
        prevCompPoints = compPoints;

        readJoystick();
        removeBall();
        setCompPaddle();
        isBallOutOfBounds();
        updateBall();


        pointTallying();
        _delay_ms(100);



    }
}
