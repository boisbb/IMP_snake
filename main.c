/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "MK60D10.h"

#define BUTTON_UP_MASK 0b100000000000000000000000000
#define BUTTON_CENT_MASK 0b100000000000
#define BUTTON_LEFT_MASK 0b1000000000000000000000000000
#define BUTTON_DOWN_MASK 0b1000000000000
#define BUTTON_RIGHT_MASK 0b10000000000

/* Macros for bit-level registers manipulation */
#define GPIO_PIN_MASK	0x1Fu
#define GPIO_PIN(x)		(((1)<<(x & GPIO_PIN_MASK)))


/* Constants specifying delay loop duration */
#define	tdelay1			10000
#define tdelay2 		20

/* LED DIODS */
// R7R4 R6R0R3R1                  R2 R5
//  11    1111   0000 0000 0000 0010 1000 0000
#define R0 0b000100000000000000000000000000
#define R1 0b000001000000000000000000000000
#define R2 0b000000000000000000001000000000
#define R3 0b000010000000000000000000000000
#define R4 0b010000000000000000000000000000
#define R5 0b000000000000000000000010000000
#define R6 0b001000000000000000000000000000
#define R7 0b100000000000000000000000000000
#define EVERY_ROW 0b111111000000000000001010000000
#define CLEAR_ROWS 0b000000000000000000000000000000

/* Directions */
#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4

#define SNAKE_LEN 6


struct snake_part{
	int x;
	int y;
	int direction;
	int direction_prev;
};

struct snake_part snake[SNAKE_LEN];
int dead_snake = 0;
int rip_move = 0;

void column_select(unsigned int col_num);


/* Configuration of the necessary MCU peripherals */
void SystemConfig() {
	/* Turn on all port clocks */
	SIM->SCGC5 = SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTE_MASK;


	/* Set corresponding PTA pins (column activators of 74HC154) for GPIO functionality */
	PORTA->PCR[8] = ( 0|PORT_PCR_MUX(0x01) );  // A0
	PORTA->PCR[10] = ( 0|PORT_PCR_MUX(0x01) ); // A1
	PORTA->PCR[6] = ( 0|PORT_PCR_MUX(0x01) );  // A2
	PORTA->PCR[11] = ( 0|PORT_PCR_MUX(0x01) ); // A3

	/* Set corresponding PTA pins (rows selectors of 74HC154) for GPIO functionality */
	PORTA->PCR[26] = ( 0|PORT_PCR_MUX(0x01) );  // R0
	PORTA->PCR[24] = ( 0|PORT_PCR_MUX(0x01) );  // R1
	PORTA->PCR[9] = ( 0|PORT_PCR_MUX(0x01) );   // R2
	PORTA->PCR[25] = ( 0|PORT_PCR_MUX(0x01) );  // R3
	PORTA->PCR[28] = ( 0|PORT_PCR_MUX(0x01) );  // R4
	PORTA->PCR[7] = ( 0|PORT_PCR_MUX(0x01) );   // R5
	PORTA->PCR[27] = ( 0|PORT_PCR_MUX(0x01) );  // R6
	PORTA->PCR[29] = ( 0|PORT_PCR_MUX(0x01) );  // R7

	/* Set corresponding PTE pins (output enable of 74HC154) for GPIO functionality */
	PORTE->PCR[28] = ( 0|PORT_PCR_MUX(0x01) ); // #EN

	PORTE->PCR[10] = ( PORT_PCR_ISF(0x01) /* Nuluj ISF (Interrupt Status Flag) */
						| PORT_PCR_IRQC(0x0A) /* Interrupt enable on failing edge */
						| PORT_PCR_MUX(0x01) /* Pin Mux Control to GPIO */
						| PORT_PCR_PE(0x01) /* Pull resistor enable... */
						| PORT_PCR_PS(0x01)); /* ...select Pull-Up */
	PORTE->PCR[11] = ( PORT_PCR_ISF(0x01) /* Nuluj ISF (Interrupt Status Flag) */
						| PORT_PCR_IRQC(0x0A) /* Interrupt enable on failing edge */
						| PORT_PCR_MUX(0x01) /* Pin Mux Control to GPIO */
						| PORT_PCR_PE(0x01) /* Pull resistor enable... */
						| PORT_PCR_PS(0x01)); /* ...select Pull-Up */
	PORTE->PCR[12] = ( PORT_PCR_ISF(0x01) /* Nuluj ISF (Interrupt Status Flag) */
						| PORT_PCR_IRQC(0x0A) /* Interrupt enable on failing edge */
						| PORT_PCR_MUX(0x01) /* Pin Mux Control to GPIO */
						| PORT_PCR_PE(0x01) /* Pull resistor enable... */
						| PORT_PCR_PS(0x01)); /* ...select Pull-Up */
	PORTE->PCR[26] = ( PORT_PCR_ISF(0x01) /* Nuluj ISF (Interrupt Status Flag) */
						| PORT_PCR_IRQC(0x0A) /* Interrupt enable on failing edge */
						| PORT_PCR_MUX(0x01) /* Pin Mux Control to GPIO */
						| PORT_PCR_PE(0x01) /* Pull resistor enable... */
						| PORT_PCR_PS(0x01)); /* ...select Pull-Up */
	PORTE->PCR[27] = ( PORT_PCR_ISF(0x01) /* Nuluj ISF (Interrupt Status Flag) */
						| PORT_PCR_IRQC(0x0A) /* Interrupt enable on failing edge */
						| PORT_PCR_MUX(0x01) /* Pin Mux Control to GPIO */
						| PORT_PCR_PE(0x01) /* Pull resistor enable... */
						| PORT_PCR_PS(0x01)); /* ...select Pull-Up */

	NVIC_ClearPendingIRQ(PORTE_IRQn);
	NVIC_EnableIRQ(PORTE_IRQn);

	SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;

	PIT_MCR = 0x00; // enable PIT clock
	PIT_TFLG0 |= PIT_TFLG_TIF_MASK; // interruption flag clear
	PIT_TCTRL0 = PIT_TCTRL_TIE_MASK; // interruption enable
	PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK; // start clock 0
	PIT_LDVAL0 = 100000;

	NVIC_ClearPendingIRQ(PIT0_IRQn);
	NVIC_EnableIRQ(PIT0_IRQn);

	/* Change corresponding PTA port pins as outputs */
	PTA->PDDR = GPIO_PDDR_PDD(0x3F000FC0);

	/* Change corresponding PTE port pins as outputs */
	PTE->PDDR = GPIO_PDDR_PDD( GPIO_PIN(28) );

	dead_snake = 0;
}


/* Variable delay loop */
void delay(int t1, int t2)
{
	int i, j;

	for(i=0; i<t1; i++) {
		for(j=0; j<t2; j++);
	}
}


void delay_letter(int t1, int t2, int value)
{
	for(int i=0; i<t1; i++) {
		for(int j=0; j<t2; j++){
			PTA->PDOR |= GPIO_PDOR_PDO(value);
		}
	}
}

void rip(int column_num){
			column_select(column_num);
	    	//PTA->PDOR |= GPIO_PDOR_PDO((R0 | R1 | R2 | R3 | R4 | R5 | R6));
	    	delay_letter(20,100, (R0 | R1 | R2 | R3 | R4 | R5 | R6));
			PTA->PDOR &= GPIO_PDOR_PDO(CLEAR_ROWS);

			column_select(column_num + 1);
			//PTA->PDOR |= GPIO_PDOR_PDO((R0 | R4));
			delay_letter(20,100, (R0 | R4));
			PTA->PDOR &= GPIO_PDOR_PDO(CLEAR_ROWS);

			column_select(column_num + 2);
			//PTA->PDOR |= GPIO_PDOR_PDO((R0 | R3 | R5));
			delay_letter(20,100, (R0 | R3 | R5));
			PTA->PDOR &= GPIO_PDOR_PDO(CLEAR_ROWS);

			column_select(column_num + 3);
			//PTA->PDOR |= GPIO_PDOR_PDO((R1 | R2 | R6));
			delay_letter(20,100, (R1 | R2 | R6));
			PTA->PDOR &= GPIO_PDOR_PDO(CLEAR_ROWS);

			column_select(column_num + 5);
			//PTA->PDOR |= GPIO_PDOR_PDO((R0 | R6));
			delay_letter(20,100, (R0 | R6));
			PTA->PDOR &= GPIO_PDOR_PDO(CLEAR_ROWS);


			column_select(column_num + 6);
			//PTA->PDOR |= GPIO_PDOR_PDO((R0 | R1 | R2 | R3 | R4 | R5 | R6));
			delay_letter(20,100, (R0 | R1 | R2 | R3 | R4 | R5 | R6));
			PTA->PDOR &= GPIO_PDOR_PDO(CLEAR_ROWS);

			column_select(column_num + 7);
			//PTA->PDOR |= GPIO_PDOR_PDO((R0 | R6));
			delay_letter(20,100, (R0 | R6));
			PTA->PDOR &= GPIO_PDOR_PDO(CLEAR_ROWS);

			column_select(column_num + 9);
			//PTA->PDOR |= GPIO_PDOR_PDO((R0 | R6));
			delay_letter(20,100, (R0 | R1 | R2 | R3 | R4 | R5 | R6));
			PTA->PDOR &= GPIO_PDOR_PDO(CLEAR_ROWS);

			column_select(column_num + 10);
			//PTA->PDOR |= GPIO_PDOR_PDO((R0 | R6));
			delay_letter(20,100, (R0 | R3));
			PTA->PDOR &= GPIO_PDOR_PDO(CLEAR_ROWS);

			column_select(column_num + 11);
			//PTA->PDOR |= GPIO_PDOR_PDO((R0 | R6));
			delay_letter(20,100, (R0 | R3));
			PTA->PDOR &= GPIO_PDOR_PDO(CLEAR_ROWS);

			column_select(column_num + 12);
			//PTA->PDOR |= GPIO_PDOR_PDO((R0 | R6));
			delay_letter(20,100, (R1 | R2));
			PTA->PDOR &= GPIO_PDOR_PDO(CLEAR_ROWS);
}

void delay_rip(int t1, int t2, int ripnum){
	for(int i=0; i<t1; i++) {
			for(int j=0; j<t2; j++){
				rip(ripnum);
			}
		}
}


/* Conversion of requested column number into the 4-to-16 decoder control.  */
void column_select(unsigned int col_num)
{
	unsigned i, result, col_sel[4];

	for (i =0; i<4; i++) {
		result = col_num / 2;	  // Whole-number division of the input number
		col_sel[i] = col_num % 2;
		col_num = result;

		switch(i) {

			// Selection signal A0
		    case 0:
				((col_sel[i]) == 0) ? (PTA->PDOR &= ~GPIO_PDOR_PDO( GPIO_PIN(8))) : (PTA->PDOR |= GPIO_PDOR_PDO( GPIO_PIN(8)));
				break;

			// Selection signal A1
			case 1:
				((col_sel[i]) == 0) ? (PTA->PDOR &= ~GPIO_PDOR_PDO( GPIO_PIN(10))) : (PTA->PDOR |= GPIO_PDOR_PDO( GPIO_PIN(10)));
				break;

			// Selection signal A2
			case 2:
				((col_sel[i]) == 0) ? (PTA->PDOR &= ~GPIO_PDOR_PDO( GPIO_PIN(6))) : (PTA->PDOR |= GPIO_PDOR_PDO( GPIO_PIN(6)));
				break;

			// Selection signal A3
			case 3:
				((col_sel[i]) == 0) ? (PTA->PDOR &= ~GPIO_PDOR_PDO( GPIO_PIN(11))) : (PTA->PDOR |= GPIO_PDOR_PDO( GPIO_PIN(11)));
				break;

			// Otherwise nothing to do...
			default:
				break;
		}
	}
}

int row_select(int row_num){
	switch(row_num){
		case 0:
			//PTA->PDOR |= GPIO_PDOR_PDO(R0);
			return R0;
			break;
		case 1:
			//PTA->PDOR |= GPIO_PDOR_PDO(R1);
			return R1;
			break;
		case 2:
			//PTA->PDOR |= GPIO_PDOR_PDO(R2);
			return R2;
			break;
		case 3:
			//PTA->PDOR |= GPIO_PDOR_PDO(R3);
			return R3;
			break;
		case 4:
			//PTA->PDOR |= GPIO_PDOR_PDO(R4);
			return R4;
			break;
		case 5:
			//PTA->PDOR |= GPIO_PDOR_PDO(R5);
			return R5;
			break;
		case 6:
			//PTA->PDOR |= GPIO_PDOR_PDO(R6);
			return R6;
			break;
		case 7:
			//PTA->PDOR |= GPIO_PDOR_PDO(R7);
			return R7;
			break;
	}
}

void light_up(int x, int y){
	column_select(y);
	PTA->PDOR |= GPIO_PDOR_PDO(row_select(x));
}

void display_snake(){
	for(int i = 0; i < SNAKE_LEN; i++){
		PTA->PDOR &= GPIO_PDOR_PDO(CLEAR_ROWS);
		light_up(snake[i].x, snake[i].y);
		delay(20,100);
	}
}

int move_snake(){
	for(int i = SNAKE_LEN - 1; i >= 0; i--){
		if(i > 1){
			snake[i].y = snake[i - 1].y;
			snake[i].x = snake[i - 1].x;
			snake[i].direction = snake[i - 1].direction;
		}
		else if(i == 1){
			snake[i].y = snake[i - 1].y;
			snake[i].x = snake[i - 1].x;
			snake[i].direction = snake[i - 1].direction_prev;
			if(snake[i - 1].direction != snake[i - 1].direction_prev)
				snake[i - 1].direction_prev = snake[i - 1].direction;
		}
		else{
			switch(snake[i].direction){
				case UP:
					snake[i].y -= 1;
					break;
				case DOWN:
					snake[i].y += 1;
					break;
				case LEFT:
					snake[i].x += 1;
					break;
				case RIGHT:
					snake[i].x -= 1;
					break;
			}
			if(snake[i].x > 7)
				snake[i].x = 0;

			if(snake[i].x < 0)
				snake[i].x = 7;
		}
	}

	for(int i = 1; i < 5; i++){
		if(snake[0].x == snake[i].x && snake[0].y == snake[i].y)
			return 1;
	}

	return 0;

}

void delay_snake(int t1, int t2)
{
	for(int i=0; i<t1; i++) {
		for(int j=0; j<t2; j++){
				display_snake();
		}
	}
}


void PORTE_IRQHandler(void) {
	if (PORTE->ISFR & BUTTON_CENT_MASK)
	{
		for(int i = 0; i < SNAKE_LEN; i++){
			snake[i].x = 4;
			snake[i].y = 7 + i;
			snake[i].direction = UP;
			snake[0].direction_prev = snake[0].direction;
		}

		dead_snake = 0;
		rip_move = 0;

		PTA->PDOR &= GPIO_PDOR_PDO(CLEAR_ROWS);
	}

	else if (PORTE->ISFR & BUTTON_RIGHT_MASK)
	{
		snake[0].direction_prev = snake[0].direction;
		snake[0].direction = RIGHT;
	}
	else if (PORTE->ISFR & BUTTON_LEFT_MASK)
	{
		snake[0].direction_prev = snake[0].direction;
		snake[0].direction = LEFT;
	}
	else if (PORTE->ISFR & BUTTON_UP_MASK)
	{
		snake[0].direction_prev = snake[0].direction;
		snake[0].direction = UP;
	}
	else if (PORTE->ISFR & BUTTON_DOWN_MASK)
	{
		snake[0].direction_prev = snake[0].direction;
		snake[0].direction = DOWN;
	}
	PORTE->ISFR = BUTTON_UP_MASK | BUTTON_CENT_MASK | BUTTON_LEFT_MASK | BUTTON_DOWN_MASK | BUTTON_RIGHT_MASK;
}


void PIT0_IRQHandler(void) {

	if(dead_snake){
		delay_rip(2, 5, rip_move++);
	}
	else{
		delay_snake(5, 7);
		if(move_snake()){
			dead_snake = 1;
		}
	}

	PTA->PDOR &= GPIO_PDOR_PDO(CLEAR_ROWS);

	PIT_TFLG0 |= PIT_TFLG_TIF_MASK;
}


int main(void)
{ 	SystemConfig();

	for(int i = 0; i < SNAKE_LEN; i++){
		snake[i].x = 4;
		snake[i].y = 8 + i;
		snake[i].direction = UP;
		snake[i].direction_prev = UP;
	}

	display_snake();

    for (;;);

    /* Never leave main */
    return 0;
}
