/*
 * File:   main.c
 * Author: James
 *
 * Created on March 24, 2017, 2:20 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <p18f4620.h>
#include <adc.h>
#include <delays.h>
#include <spi.h>
#pragma config OSC = INTIO67
#pragma config WDT = OFF
#pragma config LVP = OFF
#pragma config DEBUG = ON

#define SCLTRIS TRISCbits.TRISC3
#define SDATRIS TRISCbits.TRISC4

#define AnalogTris TRISAbits.TRISA0
#define LED LATDbits.LATD3 //Define LEDPin as PORT D Pin 1
#define LEDTris TRISDbits.TRISD3 //Define LEDTris as TRISD Pin 1

#define LCDInstructionTris7 TRISBbits.RB7    //Define Pins to control LCD Instructions
#define LCDInstruction7 LATBbits.LATB7    //Define Pins to control LCD Instructions
#define LCDInstructionTris6 TRISBbits.RB6    //Define Pins to control LCD Instructions
#define LCDInstruction6 LATBbits.LATB6    //Define Pins to control LCD Instructions
#define LCDInstructionTris5 TRISBbits.RB5    //Define Pins to control LCD Instructions
#define LCDInstruction5 LATBbits.LATB5    //Define Pins to control LCD Instructions
#define LCDInstructionTris4 TRISBbits.RB4    //Define Pins to control LCD Instructions
#define LCDInstruction4 LATBbits.LATB4    //Define Pins to control LCD Instructions
#define LCDInstructionTris3 TRISBbits.RB3    //Define Pins to control LCD Instructions
#define LCDInstruction3 LATBbits.LATB3    //Define Pins to control LCD Instructions

#define LCDInstructionTris2 TRISDbits.RD4    //Define Pins to control LCD Instructions
#define LCDInstruction2 LATDbits.LATD4    //Define Pins to control LCD Instructions
#define LCDInstructionTris1 TRISCbits.RC7    //Define Pins to control LCD Instructions
#define LCDInstruction1 LATCbits.LATC7    //Define Pins to control LCD Instructions
#define LCDInstructionTris0 TRISCbits.RC6    //Define Pins to control LCD Instructions
#define LCDInstruction0 LATCbits.LATC6    //Define Pins to control LCD Instructions

//Define Tris control bits for LCD
#define LCDEnableTris TRISDbits.TRISD7
#define LCDRnotWTris TRISDbits.TRISD6
#define LCDRegSelTris TRISDbits.TRISD5
//Define Pins for LCD control bits
#define LCDEnable LATDbits.LATD7
#define LCDRnotW LATDbits.LATD6
#define LCDRegSel LATDbits.LATD5

#define RTCwrite 0b11011110
#define RTCread 0b11011111

//Global Variables
char secondsSet;
char minutesSet;
char hoursSet;
char weekDaySet;
char dateSet;
char monthSet;
char yearSet;
char leapYearSet;

char seconds;
char secondsOnes;
char secondsTens;
char minutes;
char minutesOnes;
char minutesTens;
char hours;
char hoursOnes;
char hoursTens;
char weekDay;
char date;
char dateOnes;
char dateTens;
char month;
char monthOnes;
char monthTens;
char year;
char yearOnes;
char yearTens;

char PMset;
char normalClock;
int updateLCD;
char prevSecondsOnes;
int waitCount;
unsigned int adcVal;
char s_setTime;
char s_displayTime;
char s_setAlarm;
char s_displayAlarm;
char s_alarming;
char setAlarm; //comes from switch
char nextTime;
char switch0;
char switch1;

//Alarm variables
char alarmSeconds;
char alarmMinutes;
char alarmHour;
char alarmPM;
char alarmWeekDaySet = 1;
char alarmMonthSet = 1;
char alarmDateSet = 1;
char alarmYearSet = 0;
int alarmMinuteDiff = 0;

int i;
int j;
char brightness;
int LEDOnState;
int turnAlarmOff;
int prevAdcVal;
int diffAdcVal;


//prototypes
void LCDinstruction(char instr);
void LCDoutRes();
void LCDInit(void);
void initI2C(void);
void LCDcharOut(char ch);
void LCDoutString(rom char* string);
void showTime(void);
void writeI2C(char destReg, char data);
char readI2C(char reg);
char convertToBCD(char decimal);
char convertFromBCD(char bcd);
void Init(void);
void ADCInit(void);
unsigned int ADCRead(void);
void userSetTime(void);
void LCDoutNumber(char number);
void intInit(void);
void low_isr (void);
void high_isr (void);
void userSetAlarm(void);
void alarm(void);
void dac_write(char data);



/*
 *
 */
void main(void) {

    Init();
    ADCInit();
    LCDInit();
    initI2C();
    intInit();

    LCDInit();

    //Turn on LCD backlight and greet user
    brightness = 0xFF;
    dac_write(brightness);
    LCDoutString("The Sunshine");
    LCDinstruction(0xC0); //Go to second line
    LCDoutString("Alarm");
    Delay10KTCYx(100);

    //Set the Initial time:
    userSetTime();

    //Set Alarm
    //userSetAlarm();

    while(1){
        setAlarm = 0;

        //read buttons
        switch0 = PORTCbits.RC1;
        Delay10TCYx(10);
        switch1 = PORTCbits.RC2;
        Delay10TCYx(10);

        //See if current time is the same as alarm variables
        if(alarmHour == (hoursTens * 10 + hoursOnes)){
            if(alarmMinutes == (minutesTens * 10 + minutesOnes)){
                if((secondsTens * 10 + secondsOnes) == 0){
                    if(alarmPM == PMset){
                        setAlarm = 1;
                    }
                }
            }
        }

        if(switch0 == 1){
            LCDinstruction(0x01);           //clear home
            LCDoutString("Set New Time");
            Delay10KTCYx(40);
            userSetTime();
        }
        if(switch1 == 1){
            LCDinstruction(0x01);           //clear home
            LCDoutString("Set New Alarm");
            Delay10KTCYx(40);
            userSetAlarm();
        }


        LED = 0;
        LEDOnState = 0;

        //setAlarm gets cleared in ISR for button press
        while(setAlarm == 1){
            alarm();
            LCDInit();
            LCDoutString("I Love You");
            LCDinstruction(0xC0); //Go to second line
            LCDoutString("Forever & Always");
            Delay10KTCYx(100);
            LCDInit();
        }

        LEDOnState = 0;
        LED = 0;

        //read analog value to set lcd brightness
        adcVal = ADCRead();
        if(adcVal < 256){
            brightness = 0x00;
        }
        else if(adcVal >= 256 && adcVal < 512){
            brightness = 0x55;
        }
        else if(adcVal >= 512 && adcVal < 768){
            brightness = 0xAA;
        }
        else{
            brightness = 0xFF;
        }
        //set lcd brightness
        dac_write(brightness);
        

        seconds = readI2C(0x00);
        seconds = seconds & 0x7F;
        secondsOnes = seconds & 0x0F;
        secondsTens = (seconds & 0xF0) >> 4;
        secondsOnes = convertFromBCD(secondsOnes);
        secondsTens = convertFromBCD(secondsTens);

        minutes = readI2C(0x01);
        minutes = minutes & 0x7F;
        minutesOnes = minutes & 0x0F;
        minutesTens = (minutes & 0xF0) >> 4;
        minutesOnes = convertFromBCD(minutesOnes);
        minutesTens = convertFromBCD(minutesTens);

        hours = readI2C(0x02);
        hours = hours & 0x7F;
        hoursOnes = hours & 0x0F;
        hoursTens = (hours & 0x10) >> 4;
        PMset = (hours & 0x20) >> 5;
        normalClock = (hours & 0x40) >> 6;
        hoursOnes = convertFromBCD(hoursOnes);
        hoursTens = convertFromBCD(hoursTens);

        weekDay = readI2C(0x03);
        weekDay = weekDay & 0x07;

        date = readI2C(0x04);
        dateOnes = date & 0x0F;
        dateTens = (date & 0x30) >> 4;
        dateOnes = convertFromBCD(dateOnes);
        dateTens = convertFromBCD(dateTens);

        month = readI2C(0x05);
        monthOnes = month & 0x0F;
        monthTens = (month & 0x10) >> 4;
        monthOnes = convertFromBCD(monthOnes);
        monthTens = convertFromBCD(monthTens);

        year = readI2C(0x06);
        yearOnes = year & 0x0F;
        yearTens = (year & 0xF0) >> 4;
        yearOnes = convertFromBCD(yearOnes);
        yearTens = convertFromBCD(yearTens);



        if(secondsOnes != prevSecondsOnes){
            updateLCD = 1;
        }
        if(updateLCD == 1){
            LCDinstruction(0x02);           //go home

            LCDcharOut(hoursTens+0x30);
            LCDcharOut(hoursOnes+0x30);
            LCDcharOut(':');
            LCDcharOut(minutesTens+0x30);
            LCDcharOut(minutesOnes+0x30);
            LCDcharOut(':');
            LCDcharOut(secondsTens+0x30);
            LCDcharOut(secondsOnes+0x30);
            LCDcharOut(' ');
            if(PMset == 1){
                LCDcharOut('P');
                LCDcharOut('M');
            }
            else{
                LCDcharOut('A');
                LCDcharOut('M');
            }

            LCDinstruction(0xC0); //Go to second line

            if(weekDay == 1){
                LCDoutString("Sun");
            }
            else if(weekDay == 2){
                LCDoutString("Mon");
            }
            else if(weekDay == 3){
                LCDoutString("Tues");
            }
            else if(weekDay == 4){
                LCDoutString("Weds");
            }
            else if(weekDay == 5){
                LCDoutString("Thur");
            }
            else if(weekDay == 6){
                LCDoutString("Fri");
            }
            else if(weekDay == 7){
                LCDoutString("Sat");
            }

            LCDcharOut(' ');

            LCDcharOut(monthTens+0x30);
            LCDcharOut(monthOnes+0x30);

            LCDcharOut('/');

            LCDcharOut(dateTens+0x30);
            LCDcharOut(dateOnes+0x30);

            LCDcharOut('/');


            LCDcharOut(yearTens+0x30);
            LCDcharOut(yearOnes+0x30);

            LCDinstruction(0x80); //Go to First line
        }
        prevSecondsOnes = secondsOnes;
    }
}

void Init(){
    s_setTime = 1;
    s_displayTime = 0;
    s_setAlarm = 0;
    s_displayAlarm = 0;
    s_alarming = 0;

    TRISCbits.TRISC1 = 1; // set direction register for switch as inputs
    TRISCbits.TRISC2 = 1; // set direction register for switches as inputs

    LEDTris = 0;
    LED = 0;

    prevSecondsOnes = 0;
}

void intInit(void){
    INTCON2bits.INTEDG0 = 1; //rising edge
    INTCON2bits.RBPU = 1; //disable pullups
    INTCONbits.PEIE = 1; //enables low priority ints
    INTCONbits.INT0E = 1;
    INTCONbits.INT0IF = 0; //clear IF
    INTCONbits.GIE = 1;
    RCONbits.IPEN = 1; //turn on priorities
}

//Low Interrupts
#pragma code low_vector=0x18	//Low interrupt priority starts at 0x18
void low_interrupt(void){
    _asm GOTO low_isr _endasm
}
#pragma code

#pragma interrupt low_isr save=PROD	//Context Save
void low_isr(void){
    if(INTCONbits.INT0IF = 1){
        nextTime = 1;
        //reset flag
        INTCONbits.INT0IF = 0;
    }
}

//High Interrupts
#pragma code high_vector = 0x0008                     // code section created at 0x0018 which is the high priority interrupt
void high_interrupt(void){
        _asm GOTO high_isr _endasm                    // calling ISR
}
#pragma code

#pragma interrupt high_isr save=PROD
void high_isr(void) {
    //INT0 interrupt
    if(INTCONbits.INT0IF = 1){
        nextTime = 1;
        setAlarm = 0;

        //reset flag
        INTCONbits.INT0IF = 0;
    }

}

void ADCInit(void){
   AnalogTris = 1;       //Set A0 to be an input for the analog read
   ADCON0=0x01;         //select channel 0 and leave the rest blank
   ADCON1=0x0E;         //Normal ref voltage and setting all channels to analog
   ADCON2=0xA6;    //Right justified, meaning 10 bit conversion. Slowest default conversion rate
}

unsigned int ADCRead(void){
   ConvertADC(); //sets the ADCON0<GO> bit, starting the conversion
   while(BusyADC() == 1); //wait for the conversion to finish
   adcVal = ReadADC();  //Reads the value in the ADC register, and stores it
   return adcVal;
}

char convertToBCD(char decimal){
    char bcd = (decimal/10 * 16) + (decimal % 10);
    return bcd;
}

char convertFromBCD(char bcd){
    char decimal = (bcd/16 * 10) + (bcd % 16);
    return decimal;
}

void LCDInit(void){
    LCDInstructionTris7 = 0;         //Set Pins instruction port to be an output
    LCDInstructionTris6 = 0;         //Set Pins instruction port to be an output
    LCDInstructionTris5 = 0;         //Set Pins instruction port to be an output
    LCDInstructionTris4 = 0;         //Set Pins instruction port to be an output
    LCDInstructionTris3 = 0;         //Set Pins instruction port to be an output
    LCDInstructionTris2 = 0;         //Set Pins instruction port to be an output
    LCDInstructionTris1 = 0;         //Set Pins instruction port to be an output
    LCDInstructionTris0 = 0;         //Set Pins instruction port to be an output

    LCDEnableTris = 0;              //Set PortD control bits as output
    LCDRnotWTris = 0;               //Set PortD control bits as output
    LCDRegSelTris = 0;              //Set PortD control bits as output
    LCDRnotW = 0;                   //Keep set to write, change if you want to read busy flag

    LCDinstruction(0x38);           //2 line mode
    //LCDinstruction(0x0F);           //Block Cursor On
    //LCDinstruction(0x0E);           //Underline Cursor On
    LCDinstruction(0x0C);           //No Cursor
    LCDinstruction(0x06);           //Enables for input entry mode
    LCDinstruction(0x01);           //clear home

    Delay100TCYx(1);
}

void LCDinstruction(char instr){
    LCDRegSel = 0;
    LCDEnable = 1;
    Delay10TCYx(20);
    LCDInstruction7 = (instr & 0x80) >> 7;
    LCDInstruction6 = (instr & 0x40) >> 6;
    LCDInstruction5 = (instr & 0x20) >> 5;
    LCDInstruction4 = (instr & 0x10) >> 4;
    LCDInstruction3 = (instr & 0x08) >> 3;
    LCDInstruction2 = (instr & 0x04) >> 2;
    LCDInstruction1 = (instr & 0x02) >> 1;
    LCDInstruction0 = instr & 0x01;
    Delay10TCYx(20);
    LCDEnable = 0;
}

void LCDcharOut(char ch){
    LCDRegSel = 1;
    LCDEnable = 1;
    Delay10TCYx(20);
    LCDInstruction7 = (ch & 0x80) >> 7;
    LCDInstruction6 = (ch & 0x40) >> 6;
    LCDInstruction5 = (ch & 0x20) >> 5;
    LCDInstruction4 = (ch & 0x10) >> 4;
    LCDInstruction3 = (ch & 0x08) >> 3;
    LCDInstruction2 = (ch & 0x04) >> 2;
    LCDInstruction1 = (ch & 0x02) >> 1;
    LCDInstruction0 = ch & 0x01;
    Delay10TCYx(20);
    LCDEnable = 0;
}

void LCDoutString(rom char* string){
    int i = 0;
    while(string[i] != '\0'){
        LCDcharOut(string[i]);
        i++;
    }
}

void userSetAlarm(void){

    //Turn on LCD backlight
    brightness = 0xFF;
    dac_write(brightness);

    //initialize alarm to be on by default
    turnAlarmOff = 0;


    LCDInit();
    LCDoutString("Please Set Alarm");
    Delay10KTCYx(50);

    LCDInit();
    LCDoutString("Alarm: ");
    LCDinstruction(0xC0);
    LCDoutString("HH:MM __");
    Delay10KTCYx(30);

    LCDInit();
    nextTime = 0;
    while(nextTime == 0){
        diffAdcVal = 0;
        Delay100TCYx(20);
        adcVal = ADCRead();
        adcVal = adcVal / 78;
        if(adcVal > 12){
            adcVal = 12;
        }

        if(adcVal != prevAdcVal){
            diffAdcVal = 1;
        }
        prevAdcVal = adcVal;

        if(diffAdcVal == 1){
            if(adcVal == 0){
                LCDinstruction(0x01);           //clear home
                LCDoutString("Alarm: ");
                LCDinstruction(0xC0);
                LCDoutString("Turn Alarm Off");
            }
            else{
                LCDinstruction(0x01);           //clear home
                LCDoutString("Alarm: ");
                LCDinstruction(0xC0);
                LCDoutNumber(adcVal);
                LCDoutString(":MM __");
            }
        }
    }
    if(adcVal == 0){
        turnAlarmOff = 1;
        alarmHour = 100;
        alarmMinutes = 100;
        LCDInit();
        LCDoutString("Alarm Turned Off");
        Delay10KTCYx(40);
    }
    else{
        alarmHour = adcVal;
        Delay10KTCYx(20);

        nextTime = 0;
        while(nextTime == 0){
            adcVal = ADCRead();
            adcVal = adcVal / 17;
            if(adcVal > 59){
                adcVal = 59;
            }
            LCDinstruction(0x02);           //go home
            LCDoutString("Alarm: ");
            LCDinstruction(0xC0);
            LCDoutNumber(alarmHour);
            LCDcharOut(':');
            LCDoutNumber(adcVal);
            LCDoutString(" __");
        }
        alarmMinutes = adcVal;

        Delay10KTCYx(20);


        nextTime = 0;
        while(nextTime == 0){
            adcVal = ADCRead();
            adcVal = adcVal / 511;
            if(adcVal > 1){
                adcVal = 1;
            }
            LCDinstruction(0x02);           //go home
            LCDoutString("Alarm: ");
            LCDinstruction(0xC0);
            LCDoutNumber(alarmHour);
            LCDcharOut(':');
            LCDoutNumber(alarmMinutes);
            if(adcVal == 0){
                LCDoutString(" AM");
            }
            else if(adcVal == 1){
                LCDoutString(" PM");

            }
        }
        alarmPM = adcVal;
    
        //Turn on Alarm
        //Assign alarm to start 15 minutes before it was set
        if(alarmMinutes < 15){

            //if minutes is less than 15, adjust hour back an hour
            //edge case if alarm set to 1:00
            if(alarmHour == 1){

                if(alarmPM == 1){
                    alarmPM = 0;
                }
                else{
                    alarmPM = 1;
                }

                alarmHour = 12;
            }
            else{
                alarmHour = alarmHour - 1;
            }

            alarmMinutes = 60 + alarmMinutes - 15;
        }
        else{
            alarmMinutes = alarmMinutes - 15;
        }
    }

    Delay10KTCYx(20);
    LCDInit();
}

void userSetTime(void){

    //Turn on LCD backlight
    brightness = 0xFF;
    dac_write(brightness);
    

    writeI2C(0x07, 0b00000000); //Disable Alarm
    LCDInit();
    LCDoutString("Please Set Time");
    Delay10KTCYx(40);
    showTime();

    LCDInit();
    adcVal = ADCRead();
    adcVal = adcVal / 85;
    if(adcVal > 11){
        adcVal = 11;
    }
    adcVal = adcVal + 1;
    nextTime = 0;
    while(nextTime == 0){
        adcVal = ADCRead();
        adcVal = adcVal / 85;
        if(adcVal > 11){
            adcVal = 11;
        }
        adcVal = adcVal + 1;
        LCDinstruction(0x02);           //go home
        LCDoutNumber(adcVal);
        LCDoutString(":MM:SS __");
        LCDinstruction(0xC0);
        LCDoutString("DAY MT/DT/YR");
    }
    hoursSet = adcVal;
    Delay10KTCYx(20);

    adcVal = ADCRead();
    adcVal = adcVal / 17;
    if(adcVal > 59){
        adcVal = 59;
    }
    nextTime = 0;
    while(nextTime == 0){
        adcVal = ADCRead();
        adcVal = adcVal / 17;
        if(adcVal > 59){
            adcVal = 59;
        }
        LCDinstruction(0x02);           //go home
        LCDoutNumber(hoursSet);
        LCDcharOut(':');
        LCDoutNumber(adcVal);
        LCDoutString(":SS __");
        LCDinstruction(0xC0);
        LCDoutString("DAY MT/DT/YR");
    }
    minutesSet = adcVal;
    Delay10KTCYx(20);

    adcVal = ADCRead();
    adcVal = adcVal / 17;
    if(adcVal > 59){
        adcVal = 59;
    }
    nextTime = 0;
    while(nextTime == 0){
        adcVal = ADCRead();
        adcVal = adcVal / 17;
        if(adcVal > 59){
            adcVal = 59;
        }
        LCDinstruction(0x02);           //go home
        LCDoutNumber(hoursSet);
        LCDcharOut(':');
        LCDoutNumber(minutesSet);
        LCDcharOut(':');
        LCDoutNumber(adcVal);
        LCDoutString(" __");
        LCDinstruction(0xC0);
        LCDoutString("DAY MT/DT/YR");
    }
    secondsSet = adcVal;
    Delay10KTCYx(20);

    adcVal = ADCRead();
    adcVal = adcVal / 511;
    if(adcVal > 1){
        adcVal = 1;
    }
    nextTime = 0;
    while(nextTime == 0){
        adcVal = ADCRead();
        adcVal = adcVal / 511;
        if(adcVal > 1){
            adcVal = 1;
        }
        LCDinstruction(0x02);           //go home
        LCDoutNumber(hoursSet);
        LCDcharOut(':');
        LCDoutNumber(minutesSet);
        LCDcharOut(':');
        LCDoutNumber(secondsSet);
        if(adcVal == 0){
            LCDoutString(" AM");
        }
        else if(adcVal == 1){
            LCDoutString(" PM");

        }
        LCDinstruction(0xC0);
        LCDoutString("DAY MT/DT/YR");
    }
    PMset = adcVal;
    Delay10KTCYx(20);


    adcVal = ADCRead();
    adcVal = adcVal / 146;
    if(adcVal > 6){
        adcVal = 6;
    }
    adcVal = adcVal + 1;
    nextTime = 0;
    while(nextTime == 0){
        adcVal = ADCRead();
        adcVal = adcVal / 146;
        if(adcVal > 6){
            adcVal = 6;
        }
        adcVal = adcVal + 1;

        LCDinstruction(0x02);           //go home
        LCDoutNumber(hoursSet);
        LCDcharOut(':');
        LCDoutNumber(minutesSet);
        LCDcharOut(':');
        LCDoutNumber(secondsSet);
        if(PMset == 0){
            LCDoutString(" AM");
        }
        else if(PMset == 1){
            LCDoutString(" PM");

        }
        LCDinstruction(0xC0);

        if(adcVal == 1){
            LCDoutString("Sun ");
        }
        else if(adcVal == 2){
            LCDoutString("Mon ");
        }
        else if(adcVal == 3){
            LCDoutString("Tues ");
        }
        else if(adcVal == 4){
            LCDoutString("Weds ");
        }
        else if(adcVal == 5){
            LCDoutString("Thur ");
        }
        else if(adcVal == 6){
            LCDoutString("Fri ");
        }
        else if(adcVal == 7){
            LCDoutString("Sat ");
        }
        LCDoutString("MT/DT/YR ");
    }
    weekDaySet = adcVal;
    Delay10KTCYx(20);


    adcVal = ADCRead();
    adcVal = adcVal / 85;
    if(adcVal > 11){
        adcVal = 11;
    }
    adcVal = adcVal + 1;
    nextTime = 0;
    while(nextTime == 0){
        adcVal = ADCRead();
        adcVal = adcVal / 85;
        if(adcVal > 11){
            adcVal = 11;
        }
        adcVal = adcVal + 1;

        LCDinstruction(0x02);           //go home
        LCDoutNumber(hoursSet);
        LCDcharOut(':');
        LCDoutNumber(minutesSet);
        LCDcharOut(':');
        LCDoutNumber(secondsSet);
        if(PMset == 0){
            LCDoutString(" AM");
        }
        else if(PMset == 1){
            LCDoutString(" PM");

        }
        LCDinstruction(0xC0);

        if(weekDaySet == 1){
            LCDoutString("Sun ");
        }
        else if(weekDaySet == 2){
            LCDoutString("Mon ");
        }
        else if(weekDaySet == 3){
            LCDoutString("Tues ");
        }
        else if(weekDaySet == 4){
            LCDoutString("Weds ");
        }
        else if(weekDaySet == 5){
            LCDoutString("Thur ");
        }
        else if(weekDaySet == 6){
            LCDoutString("Fri ");
        }
        else if(weekDaySet == 7){
            LCDoutString("Sat ");
        }

        LCDoutNumber(adcVal);
        LCDoutString("/DT/YR ");
    }
    monthSet = adcVal;
    Delay10KTCYx(20);


    adcVal = ADCRead();
    adcVal = adcVal / 33;
    if(adcVal > 30){
        adcVal = 30;
    }
    adcVal = adcVal + 1;
    nextTime = 0;
    while(nextTime == 0){
        adcVal = ADCRead();
        adcVal = adcVal / 33;
        if(adcVal > 30){
            adcVal = 30;
        }
        adcVal = adcVal + 1;

        LCDinstruction(0x02);           //go home
        LCDoutNumber(hoursSet);
        LCDcharOut(':');
        LCDoutNumber(minutesSet);
        LCDcharOut(':');
        LCDoutNumber(secondsSet);
        if(PMset == 0){
            LCDoutString(" AM");
        }
        else if(PMset == 1){
            LCDoutString(" PM");

        }
        LCDinstruction(0xC0);

        if(weekDaySet == 1){
            LCDoutString("Sun ");
        }
        else if(weekDaySet == 2){
            LCDoutString("Mon ");
        }
        else if(weekDaySet == 3){
            LCDoutString("Tues ");
        }
        else if(weekDaySet == 4){
            LCDoutString("Weds ");
        }
        else if(weekDaySet == 5){
            LCDoutString("Thur ");
        }
        else if(weekDaySet == 6){
            LCDoutString("Fri ");
        }
        else if(weekDaySet == 7){
            LCDoutString("Sat ");
        }

        LCDoutNumber(monthSet);
        LCDcharOut('/');
        LCDoutNumber(adcVal);
        LCDoutString("/YR ");
    }
    dateSet = adcVal;
    Delay10KTCYx(20);


    adcVal = ADCRead();
    adcVal = adcVal / 10;
    if(adcVal > 99){
        adcVal = 99;
    }
    nextTime = 0;
    while(nextTime == 0){
        adcVal = ADCRead();
        adcVal = adcVal / 10;
        if(adcVal > 99){
            adcVal = 99;
        }

        LCDinstruction(0x02);           //go home
        LCDoutNumber(hoursSet);
        LCDcharOut(':');
        LCDoutNumber(minutesSet);
        LCDcharOut(':');
        LCDoutNumber(secondsSet);
        if(PMset == 0){
            LCDoutString(" AM");
        }
        else if(PMset == 1){
            LCDoutString(" PM");

        }
        LCDinstruction(0xC0);

        if(weekDaySet == 1){
            LCDoutString("Sun ");
        }
        else if(weekDaySet == 2){
            LCDoutString("Mon ");
        }
        else if(weekDaySet == 3){
            LCDoutString("Tues ");
        }
        else if(weekDaySet == 4){
            LCDoutString("Weds ");
        }
        else if(weekDaySet == 5){
            LCDoutString("Thur ");
        }
        else if(weekDaySet == 6){
            LCDoutString("Fri ");
        }
        else if(weekDaySet == 7){
            LCDoutString("Sat ");
        }

        LCDoutNumber(monthSet);
        LCDcharOut('/');
        LCDoutNumber(dateSet);
        LCDcharOut('/');
        LCDoutNumber(adcVal);
        LCDcharOut(' ');
    }
    yearSet = adcVal;
    Delay10KTCYx(20);

    //Set the time
        //Time Settings
        if(yearSet % 4 == 0){
            leapYearSet = 1;
        }
        else{
            leapYearSet = 0;
        }

        //write seconds
        secondsSet = convertToBCD(secondsSet);
        secondsSet = secondsSet | 0x80;
        writeI2C(0x00, secondsSet);

        //write minutes
        minutesSet = convertToBCD(minutesSet);
        writeI2C(0x01, minutesSet);

        //write hours
        hoursSet = convertToBCD(hoursSet);
        hoursSet = hoursSet | (PMset << 5);
        hoursSet = hoursSet | 0x40; //Set to 12 hour mode
        writeI2C(0x02, hoursSet);

        //write Weekday
        writeI2C(0x03, weekDaySet);

        //write date
        dateSet = convertToBCD(dateSet);
        writeI2C(0x04, dateSet);

        //write month
        monthSet = convertToBCD(monthSet);
        if(leapYearSet == 1){
            monthSet = monthSet | 0x20; //Set to be a leapYear
        }
        writeI2C(0x05, monthSet);

        //write year
        yearSet = convertToBCD(yearSet);
        writeI2C(0x06, yearSet);

}

void alarm(void){
    LCDInit();
    LCDoutString("Good Morning");
    LCDinstruction(0xC0); //Go to second line
    LCDoutString("Kylie!");

    while(setAlarm == 1){
        //Get current minutes and hours
        minutes = readI2C(0x01);
        minutes = minutes & 0x7F;
        minutesOnes = minutes & 0x0F;
        minutesTens = (minutes & 0xF0) >> 4;
        minutesOnes = convertFromBCD(minutesOnes);
        minutesTens = convertFromBCD(minutesTens);

        hours = readI2C(0x02);
        hours = hours & 0x7F;
        hoursOnes = hours & 0x0F;
        hoursTens = (hours & 0x10) >> 4;
        PMset = (hours & 0x20) >> 5;
        normalClock = (hours & 0x40) >> 6;
        hoursOnes = convertFromBCD(hoursOnes);
        hoursTens = convertFromBCD(hoursTens);

        //alarmMinutes < 45
        if(alarmMinutes < 45){
            if((minutesTens * 10 + minutesOnes) == alarmMinutes){ //alarm just went off
                LED = 1;
                Delay10TCYx(5);
                LED = 0;
                Delay10TCYx(95);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+1){ //1 minute has passed
                LED = 1;
                Delay10TCYx(10);
                LED = 0;
                Delay10TCYx(90);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+2){
                LED = 1;
                Delay10TCYx(20);
                LED = 0;
                Delay10TCYx(80);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+3){
                LED = 1;
                Delay10TCYx(30);
                LED = 0;
                Delay10TCYx(70);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+4){
                LED = 1;
                Delay10TCYx(40);
                LED = 0;
                Delay10TCYx(60);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+5){
                LED = 1;
                Delay10TCYx(50);
                LED = 0;
                Delay10TCYx(50);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+6){
                LED = 1;
                Delay10TCYx(60);
                LED = 0;
                Delay10TCYx(40);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+7){
                LED = 1;
                Delay10TCYx(70);
                LED = 0;
                Delay10TCYx(30);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+8){
                LED = 1;
                Delay10TCYx(80);
                LED = 0;
                Delay10TCYx(20);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+9){
                LED = 1;
                Delay10TCYx(90);
                LED = 0;
                Delay10TCYx(10);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+10){
                LED = 1;
                Delay10TCYx(95);
                LED = 0;
                Delay10TCYx(5);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+11){
                LED = 1;
                Delay10TCYx(100);
                LED = 0;
                Delay10TCYx(1);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+12){
                LED = 1;
                Delay10TCYx(150);
                LED = 0;
                Delay10TCYx(1);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+13){
                LED = 1;
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+14){
                LED = 1;
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+15){
                LED = 1;
                LEDOnState = 1;
            }
        }
        else{ //alarm occurs > 45 minutes
            // Minutes time overflows to next hour, update alarmtime to continue through the flow
            if((minutesTens * 10 + minutesOnes) == 0){
                alarmMinutes = -60 + alarmMinutes;
            }
            if((minutesTens * 10 + minutesOnes) == alarmMinutes){ //alarm just went off
                LED = 1;
                Delay10TCYx(5);
                LED = 0;
                Delay10TCYx(95);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+1){ //1 minute has passed
                LED = 1;
                Delay10TCYx(10);
                LED = 0;
                Delay10TCYx(90);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+2){
                LED = 1;
                Delay10TCYx(20);
                LED = 0;
                Delay10TCYx(80);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+3){
                LED = 1;
                Delay10TCYx(30);
                LED = 0;
                Delay10TCYx(70);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+4){
                LED = 1;
                Delay10TCYx(40);
                LED = 0;
                Delay10TCYx(60);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+5){
                LED = 1;
                Delay10TCYx(50);
                LED = 0;
                Delay10TCYx(50);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+6){
                LED = 1;
                Delay10TCYx(60);
                LED = 0;
                Delay10TCYx(40);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+7){
                LED = 1;
                Delay10TCYx(70);
                LED = 0;
                Delay10TCYx(30);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+8){
                LED = 1;
                Delay10TCYx(80);
                LED = 0;
                Delay10TCYx(20);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+9){
                LED = 1;
                Delay10TCYx(90);
                LED = 0;
                Delay10TCYx(10);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+10){
                LED = 1;
                Delay10TCYx(95);
                LED = 0;
                Delay10TCYx(5);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+11){
                LED = 1;
                Delay10TCYx(100);
                LED = 0;
                Delay10TCYx(1);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+12){
                LED = 1;
                Delay10TCYx(150);
                LED = 0;
                Delay10TCYx(1);
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+13){
                LED = 1;
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+14){
                LED = 1;
            }
            else if((minutesTens * 10 + minutesOnes) == alarmMinutes+15){
                LED = 1;
                LEDOnState = 1;
            }
        }
        if(LEDOnState == 1){
            LED = 1;
        }
    }
}

void LCDoutNumber(char number){
    char ones = number % 10;
    char tens = number / 10;
    tens = tens % 10;

    LCDcharOut(tens + 0x30);
    LCDcharOut(ones + 0x30);

}

void showTime(void){
    LCDInit();
    LCDoutString("HH:MM:SS AM");
    LCDinstruction(0xC0);
    LCDoutString("DAY MT/DT/YR");
}

void dac_write(char data){
	unsigned int output = 0x0000;
	unsigned char temp;
	signed char x;

	TRISDbits.TRISD0 = 0; // set direction register
	TRISDbits.TRISD1 = 0;
	TRISDbits.TRISD2 = 0;

	PORTDbits.RD0 = 0;   // data to 0
	PORTDbits.RD1 = 0;   // clock to 0
	PORTDbits.RD2 = 1;   // enable to 1

	temp = data;         // format data
	data >>= 4;
	data |= 0xF0;
	output = data;
	output <<= 8;
	data = temp;
	data <<= 4;
	output |= (data & 0x00FF);

	PORTDbits.RD2 = 0;  //enable low
	for (x = 15; x > -1; x--){   // send 16 bits of data
		PORTDbits.RD0 = (output >> x) & 0x01;
		PORTDbits.RD1 = 1;
		PORTDbits.RD1 = 0;
		PORTDbits.RD0 = 0;
	}
	PORTDbits.RD2 = 1;  //enable high
}

void initI2C(void){
    //To initialize the ports set the output resisters to 0 and the tristate registers to 1 which disables the outputs and allows them to be pulled high by the resistors.
    SCLTRIS = 1;
    SDATRIS = 1;
    PIR1bits.SSPIF = 0;
    SSPADD = (8000000 / (4*100000)) - 1;
    SSPSTAT = 0x80;
    SSPCON1 = 0x28;
    SSPCON2 = 0x00;
}


void startI2C(void){
    SSPCON2bits.SEN = 1;
    while(SSPCON2bits.SEN);
}

void waitI2C(void){
    while(PIR1bits.SSPIF == 0);
    PIR1bits.SSPIF = 0;
}

void sendCharI2C(char data){
    while(SSPSTATbits.BF);
    SSPBUF = data;
}

void checkAckI2C(void){
    if(SSPCON2bits.ACKSTAT){
        LCDoutString("Failed:");
        LCDinstruction(0xC0);
        LCDoutString("Restart Clock");
        while(1);
    }
}

void turnOnRTC(void){
    //Starts RTC by setting ST bit high
    sendCharI2C(0x80);
}

void restartI2C(void){
    SSPCON2bits.RSEN = 1;
    while(SSPCON2bits.RSEN);
}

void enableReceiveI2C(void){
    SSPCON2bits.RCEN = 1;
    while(SSPCON2bits.RCEN);
}

void acknowledgeI2C(char ack){
    //acknowledge or not
    SSPCON2bits.ACKDT = ack;
    //Turn on acknowledgement
    SSPCON2bits.ACKEN = 1;
}

void stopI2C(void){
    SSPCON2bits.SEN = 0;
    while(SSPCON2bits.SEN = 0);
}

char readDataI2C(void){
    return SSPBUF;
}

char readI2C(char reg){
    startI2C();
    waitI2C();
    sendCharI2C(RTCwrite); //Tell RTC we are going to write
    waitI2C();
    checkAckI2C();
    sendCharI2C(reg); //Tell RTC which register to read from
    waitI2C();
    checkAckI2C();
    restartI2C();
    waitI2C();
    sendCharI2C(RTCread); //Tell RTC we are going to read
    waitI2C();
    checkAckI2C();
    enableReceiveI2C();
    waitI2C();
    acknowledgeI2C(1);
    waitI2C();
    stopI2C();
    return readDataI2C();
}

void writeI2C(char destReg, char data){
    startI2C();
    waitI2C();
    sendCharI2C(RTCwrite); //Tell RTC we are going to write
    waitI2C();
    checkAckI2C();
    sendCharI2C(destReg); //Tell RTC which register to write to
    waitI2C();
    checkAckI2C();
    sendCharI2C(data);
    waitI2C();
    checkAckI2C();
    turnOnRTC();
    waitI2C();
}