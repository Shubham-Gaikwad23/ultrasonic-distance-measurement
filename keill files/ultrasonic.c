#include<reg51.h>		//Header file inclusion for 8051

void delay(unsigned int rtime);
void lcdcmd(unsigned char DATA);
void initialize(void);
void lcddat(unsigned char DATA);
void display_lcd(unsigned char location, unsigned char *d);
void send_pulse(void);
void lcd_number(int val);
void get_range(void);

#define LCDdata P1		//Declaring LCDdata	

sbit trig=P3^5;		//timer 1
sbit echo=P3^2;		//INTR 0

sbit LCDrs = P2^0;		//The Register select Pin
sbit LCDrw = P2^1;		//The Read/Write Pin
sbit LCDen = P2^2;		//The Enable Pin
int lastRange = 0;		//remember last reading 
int count = 0;

void delay(unsigned int rtime)
{
	TMOD = 0x19;				//set timer1 in 16-bit mode
	switch(rtime)
	{
		case 1:
			TH1 = 0xDB;			//Load 0xDBFF in TH, TL
			TL1 = 0xFF;			//Value calculated for 10ms delay
		break;
		case 2:
			TH1 = 0x48;			//Load 0x4801 in TH, TL
			TL1 = 0x01;			//Value calculated for 20ms delay
		break;
		case 3:
			TH1 = 0x00;			//Load 0x0009 in TH, TL
			TL1 = 0x09;			//Value calculated for 10us delay
		break;
		default:
			return; 			//wrong parameter input rtime
	}	

	TR1 = 1;			//Start timer in 16-bit mode 1
	while(TF1 == 0);	//wait for flag
	TR1 = 0;			//stop timer
	TF1 = 0;			//reset flag
}

void lcdcmd(unsigned char DATA)
{
  	LCDrs=0;
	LCDrw=0;
	LCDen=1;			//Strobe the enable pin
	LCDdata = DATA;			//Put the value on the pins
	LCDrs=0;
	LCDrw=0;
	LCDen=0;
}

void initialize(void) 
{
	lcdcmd(0x30);		//1 line and 5x7 matrix 
	delay(1);

	lcdcmd(0x38);		//2 line and 5x7 matrix 
	delay(1);

	lcdcmd(0x0c);		//Display on, cursor off
	delay(1);

	lcdcmd(0x01);		//Clear display Screen 
	delay(1);

	lcdcmd(0x06);		//shift cursor to right
	delay(1);
}

void lcddat(unsigned char DATA)
{
	LCDrs = 1;
	LCDrw = 0;
	LCDen = 1;		//Strobe the enable pin
	LCDdata = DATA;		//Put the value on the pins
	LCDrs = 1;
	LCDrw = 0;
	LCDen = 0;
}

void display_lcd(unsigned char location, unsigned char *d)
{
	lcdcmd(0x00 | location);
	delay(1);				//10mS delay generation
	while(*d)
 	{
		lcddat(*d++);
  		delay(1);				//10mS delay generation
	}
}

void send_pulse(void) //to generate 10 microseconds delay
{
	TH0=0x00;
	TL0=0x00;
	trig=0;
	trig=1;
 	delay(3);
 	trig=0;
}

void lcd_number(int val)			// Function to display number 
{
	int i=3;
	char str[7]={"0000 CM"};
	while(val>0)
	{
		str[i]=0x30 | val%10;
		val=val/10;
		i--;
	}
display_lcd(0xC5,str);
}

void serial_comm(int range)
{
	char str[20]="Object out of range";
	int i;
	
	if(range>0)
	{
		for(i=2; range>0; i--)
		{
			str[i]=0x30 | range%10;
			range=range/10;
		}
		str[3]='\0';
	}

	TMOD = 0x29;				//Send characters
	TH1 = -3;
	SCON = 0x40;
	TR1 = 1;
	
	for(i=0; i<count; i++)		// clear old data sent
	{
		SBUF = 8;
		while(TI==0);
		TI=0;
	}
	for(i=0; str[i]; i++)
	{
		SBUF = str[i];
		while(TI==0);
		TI=0;
	}
	
	TR1=0;
	TF1=0;
	count = i;
}



void get_range(void)
{
	int range=0;
	int timerval;
	send_pulse();
	while(INT0==0);
	while(INT0==1);
	timerval = TH0;
	timerval = (timerval << 8) | TL0;
	TH0=0xFF;
	TL0=0xFF;
 	if(timerval<35000)  //Maximum 38000us work at higher levels
		range=timerval/59;
	else
		range = 0;
	if(lastRange != range)
	{
		lcd_number(range);
		serial_comm(range);
		lastRange = range;
	}
}

 void main(void)
{
	initialize();			  		//initilaze LCD
	display_lcd(0x80,"  OBSTACLE  AT  ");		//Display character String from location specified 
	TMOD=0x09;					//timer0 in 16 bit mode with gate enable
  TR0=1;						//timer run enabled
  TH0=0x00;
	TL0=0x00;
  echo = 1;					//setting pin P3.2 as input 
	while(1)
	{
		get_range();
		delay(2);
	}
}
