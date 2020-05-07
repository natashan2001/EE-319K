// SpaceInvaders.c
// Runs on LM4F120/TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the EE319K Lab 10

// Last Modified: 1/17/2020 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* This example accompanies the books
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2019

   "Embedded Systems: Introduction to Arm Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2019

 Copyright 2019 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// special weapon fire button connected to PE1
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
// LED on PB5

// Backlight (pin 10) connected to +3.3 V
// MISO (pin 9) unconnected
// SCK (pin 8) connected to PA2 (SSI0Clk)
// MOSI (pin 7) connected to PA5 (SSI0Tx)
// TFT_CS (pin 6) connected to PA3 (SSI0Fss)
// CARD_CS (pin 5) unconnected
// Data/Command (pin 4) connected to PA6 (GPIO), high for data, low for command
// RESET (pin 3) connected to PA7 (GPIO)
// VCC (pin 2) connected to +3.3 V
// Gnd (pin 1) connected to ground

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "ST7735.h"
#include "Print.h"
#include "Random.h"
#include "PLL.h"
#include "ADC.h"
#include "Images.h"
#include "Sound.h"
#include "Timer0.h"
#include "Timer1.h"
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
void Delay100ms(uint32_t count); // time delay in 0.1 seconds

#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define NUMBER_OF_STRING 2
#define MAX_STRING_SIZE 265

// Initialize Port F so PF1, PF2 and PF3 are heartbeats
void SysTick_Handler (void);
void movehookem (int laser, int num);
void newspawn (int count);
void moveenemy (void);
void drawcollision(int i, int laser, int which);
void killenemy(int i);
void respawnlaser(void);
void startscreen(void);
void endscreens(void);
void PortF_Init(void){
	 SYSCTL_RCGCGPIO_R|= 0x00000020;     // 1) activate clock for Port F
  while((SYSCTL_RCGCGPIO_R&0x20) == 0){};           // allow time for clock to start
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0
  // only PF0 needs to be unlocked, other bits can't be locked
  GPIO_PORTF_AMSEL_R = 0x00;        // 3) disable analog on PF
  GPIO_PORTF_PCTL_R = 0x00000000;   // 4) PCTL GPIO on PF4-0
  GPIO_PORTF_DIR_R = 0x0E;          // 5) PF4,PF0 in, PF3-1 out
  GPIO_PORTF_AFSEL_R = 0x00;        // 6) disable alt funct on PF7-0
  GPIO_PORTF_PUR_R = 0x11;          // enable pull-up on PF0 and PF4
  GPIO_PORTF_DEN_R = 0x1F;          // 7) enable digital I/O on PF4-0
}
void PortE_Init(void){
	SYSCTL_RCGCGPIO_R |= 0x10;			// 1) activate clock for Port E 
	while((SYSCTL_PRGPIO_R&0x10) == 0){}; 
	GPIO_PORTE_DIR_R &= 0x3;			// 
	GPIO_PORTE_AFSEL_R &= ~0x3;		 //
	GPIO_PORTE_DEN_R |= 0x3;			// 
	GPIO_PORTE_AMSEL_R &= ~0x3;		 // 
}
void PortB_Init(void){
	SYSCTL_RCGCGPIO_R |= 0x02;			// 1) activate clock for Port B
	while((SYSCTL_PRGPIO_R&0x01) == 0){}; 
	GPIO_PORTE_DIR_R &= 0x00;			// 
	GPIO_PORTE_AFSEL_R &= ~0x01;		 //
	GPIO_PORTE_DEN_R |= 0x01;			// 
	GPIO_PORTE_AMSEL_R &= ~0x01;		 // 
}

void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0; // disable SysTick during setup
  NVIC_ST_RELOAD_R = 2666666;  // maximum reload value //30HZ
  NVIC_ST_CURRENT_R = 0;                // any write to current clears it
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x20000000;	
  NVIC_ST_CTRL_R = 0x07;
}

uint32_t ADCmail;
uint32_t ADCstatus = 1;
uint32_t Data;        // 12-bit ADC
uint32_t Position;  

void SysTick_Handler (void){
	GPIO_PORTF_DATA_R ^= 0x04;
	Data = ADC_In();
	ADCmail = Data;
	ADCstatus = 1;
}
uint32_t change;

uint32_t Convert(uint32_t input){
	uint32_t posy;
	uint32_t last;
	if(input<10)
	{
		posy= 0; 
	}
	if ((input>100)&& input<2600){
   posy=(((160*input)/4096)); 
	}
	else
	{
		posy=125;
	}
  if (last!=posy){
		change=1;
		last=posy;
	}
	return posy;
}
//structs
struct sprite{
	uint32_t X,Y;	
	const unsigned short *image;
	uint32_t W,H;
	int32_t life;
};
typedef  struct sprite sprite_t;
sprite_t hookem[5];
sprite_t enemy[4];
sprite_t bevo;
void Init(void)
{
	bevo.X=10;
	bevo.Y=30;
	bevo.image=Bunker0;
	bevo.W=22;
	bevo.H=14;
	bevo.life=1;
  ST7735_DrawBitmap(bevo.X,bevo.Y,bevo.image,bevo.W,bevo.H);
	for (int i=0;i<6;i++)
	{
		hookem[i].X=bevo.H/2;
		hookem[i].Y=bevo.W+1;
		hookem[i].image=PlayerShip0;
		hookem[i].W=11;
		hookem[i].H=16;
		hookem[i].life=1;
		ST7735_DrawBitmap(hookem[i].X,hookem[i].Y,hookem[i].image,hookem[i].W,hookem[i].H);
	}
	enemy[0].X=0;
	enemy[0].Y=160;
	enemy[0].image=SmallEnemy10pointA;
	enemy[0].W=20;
	enemy[0].H=11;
	enemy[1].X=20;
	enemy[1].Y=160;
	enemy[1].image=SmallEnemy10pointA;
	enemy[1].W=20;
	enemy[1].H=11;
	enemy[2].X=50;
	enemy[2].Y=160;
	enemy[2].image=SmallEnemy10pointA;
	enemy[2].W=20;
	enemy[2].H=11;
	enemy[3].X=100;
	enemy[3].Y=160;
	enemy[3].image=SmallEnemy10pointA;
	enemy[3].W=20;
	enemy[3].H=11;
	for (int i=0; i<4; i++)
	{
		enemy[i].life=1;
		ST7735_DrawBitmap(enemy[i].X,enemy[i].Y,enemy[i].image,enemy[i].W,enemy[i].H);	
	}

}
int howmany;
int lasers=0;
int totallasers=0;
int enenynum=4;
int lasti=0;
int language;
int score=0;
int deaths=0;
int totalenemies=0;
int enemieskilled=0;
int main(void){
  DisableInterrupts();
  PLL_Init(Bus80MHz);       // Bus clock is 80 MHz 
	Random_Init(NVIC_ST_CURRENT_R);
  Output_Init();
  SysTick_Init();
	PortE_Init();
	ST7735_InitR(INITR_BLACKTAB);
	PortF_Init(); 
	PortB_Init();
	Init();
  ADC_Init();	
	Sound_Init();
	EnableInterrupts();
	Random_Init(NVIC_ST_CURRENT_R);  
  ST7735_FillScreen(0x0000);   // set screen to black
	ST7735_SetTextColor(0xFFFF);
	startscreen();
  int boost;
	int lasttime=0;
	int answer;
  while(1){
		if(ADCstatus == 1)
		{
			howmany++;
			ADCstatus = 0;
			answer = Convert(ADCmail);
			if (change)
			{
				bevo.X=answer;
				hookem[lasers].X=bevo.X;
			}
				for(int i=0;i<lasers;i++)
					{
						if(hookem[i].Y!=150 && hookem[i].life==1)
						{
							movehookem(i, howmany);
						}
						if (hookem[i].Y>=150)
						{
							hookem[i].life=0;
						}
				   }
					respawnlaser();
				int yes;
				yes=(GPIO_PORTF_DATA_R & 0x01);
				if (yes==0x00)
				{
					while (GPIO_PORTF_DATA_R==0){};
						if (lasers<6)
						{
				    	lasers++;
							Sound_Shoot();
						}
						else
						{
							totallasers++;
						}
				}
				ST7735_SetCursor(0,0);
				ST7735_OutString("Boost=");
				if(boost)
				{ST7735_OutString("Yes");}
				else
				{ST7735_OutString("No");}
				ST7735_SetCursor(12,0);
				ST7735_OutString("Lives=");
				LCD_OutDec(10-deaths);
	   		ST7735_FillRect(0,20,128,40,0x0000);
			  ST7735_FillRect(0,135,128,25,0x0000);
				ST7735_DrawBitmap(bevo.X,bevo.Y,bevo.image,bevo.W,bevo.H);
			
      if ((GPIO_PORTF_DATA_R &0x10)==0)
			{
				boost=1;
			  lasttime=howmany;
			}
			if((howmany-lasttime)<=200)
			{
				boost=1;
			}
			else{boost=0;}
			if (boost)
			{
				if (howmany%2==0)
		  	{
				   moveenemy();
			  }
				if(howmany%10==0)
			  {
				   ST7735_FillRect(0,40,128,100,0x0000);
			  }
			}
			else 
			{
			if (howmany%5==0)
			{
				moveenemy();
			}
			if(howmany%25==0)
			{
				ST7735_FillRect(0,40,128,100,0x0000);
			}
		}
			
		}
 }
}
void movehookem (int laser, int num)
{
	int i=0;
	if (num%1==0){
	hookem[laser].Y=hookem[laser].Y+3;
	for (i=0; i<4; i++)
	{
		for (int j=-16; j<16; j++)
		{
			for (int k=0; k<10;k++)
			{
			if ((hookem[laser].X==(enemy[i].X+j))&&((hookem[laser].Y+k)==enemy[i].Y)&&(enemy[i].life==1)&&(hookem[laser].life==1))
			{
				 drawcollision(i, laser,0);
				 killenemy(i);
				 hookem[laser].life=0;
				enemieskilled++;
				 Sound_Killed();
			}
		}
		}
	}
  if (hookem[laser].life==1)
	{
		ST7735_FillRect(hookem[laser].X,hookem[laser].Y-18,hookem[laser].W,hookem[laser].H+2,0x0000);
		ST7735_DrawBitmap(hookem[laser].X, hookem[laser].Y, hookem[laser].image, hookem[laser].W,hookem[laser].H);
	}
	if (hookem[laser].Y>=150)
	{
		hookem[laser].life=0;
	}
	if (hookem[laser].life==0)
	{
		drawcollision(i,laser,2);
	}
  }
}
void newspawn (int i)
{
	enenynum++;	
	totalenemies++;
	Sound_Fastinvader1();
	int last=0;
	int m;
	for (int l=-10; l<10;l++)
	{
		if ((last+l)!=m)
    {
			m = (Random())%115;
		}
	}		
	last=m;
	enemy[i].Y=160;
	for (int j=0; j<4; j++)
	{
		if ((enemy[j].X+10==m) || (enemy[j].X-10==m))
		{
			m=m+15;
		}
		if(enemy[j].Y>=150)
		{
			enemy[j].Y=190;
			if(enemy[j].X==m)
			{
				enemy[j].X=m+20;
			}
		}
	}
	if (m>115)
	{
		m=115;
	}
	enemy[i].X=m;
	m= howmany%2;
	if (m==0)
	{
		enemy[i].image=SmallEnemy10pointA;
	}
	if (m==1)
	{
		enemy[i].image=SmallEnemy10pointB;
	}
	enemy[i].W=20;
	enemy[i].H=11;
	enemy[i].life=1;
	ST7735_DrawBitmap(enemy[i].X,enemy[i].Y,enemy[i].image,enemy[i].W, enemy[i].H);
	return;
}
void moveenemy (void)
{
	for(int i=0; i<enenynum; i++)
	{
		if (enemy[i].Y<=50)
		{
			killenemy(i);
			deaths++;
			if(deaths>=10)
			{
				endscreens();
			}
		}
		if (enemy[i].life==1)
		{
			enemy[i].Y=enemy[i].Y-2;
			ST7735_FillRect(enemy[i].X, enemy[i].Y+2, enemy[i].W,enemy[i].H, 0x0000);
			ST7735_DrawBitmap(enemy[i].X, enemy[i].Y, enemy[i].image, enemy[i].W,enemy[i].H);
		}
	}
	return;
}
void drawcollision(int i, int laser, int which)
{
  ST7735_FillRect(hookem[laser].X,hookem[laser].Y-16,hookem[laser].W+7,hookem[laser].H+20,0x0000);
	if (which==2)
	{
		return;
	}
	 ST7735_FillRect(enemy[i].X,enemy[i].Y-9,enemy[i].W+7,enemy[i].H+10,0x0000);
}
void killenemy(int i)
{
	enenynum--;
	enemy[i].life=0;
  ST7735_FillRect(enemy[i].X-2,enemy[i].Y-12,enemy[i].W+8,enemy[i].H+5,0x0000);
	if (enenynum<4)
	{
	  newspawn(i);
	}
}
void respawnlaser()
{
	for(int i=0; i<totallasers;i++)
	{
		if (hookem[i].Y>=150)
		{
			hookem[i].life=0;
		}
		if(hookem[i].life==1)
		{
			movehookem(i, howmany);
		}
		if(hookem[i].life==0)
		{
			totallasers=0;
			hookem[i].X=bevo.X;
			hookem[i].Y=16;
			hookem[i].life=1;
		}
	}
}

char arr[NUMBER_OF_STRING][MAX_STRING_SIZE] =
{ "Anleitungen: Bevo hat ein Minute um Darrell K Royal Stadion zu gehen und er braucht ihre Hilfe. Benutzen den linken Button, um den Feindin mit Ihren Hook ‘em Symbol erschießen und benutzen der recht Button erhöhen. Nutzen der Gleitstück zu Linke und rechts bewegen.",
  "Instructions: Bevo has one minute to reach Darrell K Royal Stadium and you have to help him! Use the left button to shoot oncoming enemies with your Hook ‘em signs and use the right button to boost. Use the slider to move left and right!",
};
void startscreen(void)
{
	int flag=0;
	ST7735_DrawBitmap(20, 90, titlepage , 84,77);
	while (flag!=1){
	if((GPIO_PORTF_DATA_R&~0x10)==0x00)
	{
		language=1;
		flag=1;
		ST7735_FillScreen(0x0000);
		int y=0;
		ST7735_SetCursor(0,0);
		for(int i=0; i<MAX_STRING_SIZE; i++)
		{
			ST7735_OutChar(arr[0][i]);
			if(i%20==19)
			{
			  y++;
				ST7735_SetCursor(0,y);
			}
		}
	}
	if((GPIO_PORTF_DATA_R&~0x01)==0x00)
	{
		language=0;
		flag=1;
		ST7735_FillScreen(0x0000);
		int y=0;
		ST7735_SetCursor(0,0);
		for(int i=0; i<MAX_STRING_SIZE; i++)
		{
			ST7735_OutChar(arr[1][i]);
			if(i%20==19)
			{
			  y++;
				ST7735_SetCursor(0,y);
			}
		}
	}
}
	while((GPIO_PORTF_DATA_R&~0x01)!=0x00||(GPIO_PORTF_DATA_R&~0x10)!=0x00)
	{}
	ST7735_FillScreen(0x0000);
	return;
}
void endscreens (void)
{
	Sound_End();
	ST7735_FillScreen(0x0000);
	ST7735_DrawBitmap(20, 60, scoreboard , 64, 53);
	ST7735_SetCursor(10,4);
	LCD_OutDec(enemieskilled);
	ST7735_SetCursor(15,5);
	LCD_OutDec(totalenemies+howmany%10000);
	while(1){};
}

	
void Delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}
