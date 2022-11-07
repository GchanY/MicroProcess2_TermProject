#include <mega128.h>
#include <delay.h>
#include <math.h>
#include <stdio.h>
#include "lcd.h"

#define SERVO_PIN_LOW() PORTD &= ~(1<<PORTD7)
#define SERVO_PIN_HIGH() PORTD |= (1<<PORTD7)

#define ADC_VREF_TYPE 0x00
#define ADC_AVCC_TYPE 0x40
#define ADC_RES_TYPE 0x80
#define ADC_2_56_TYPE 0xc0

unsigned int T2_DUTY_TIME_us;
unsigned int T2_CYCLE_TIME_us;

unsigned int T2_DUTY_TIME_cnt_us;
unsigned int T2_CYCLE_TIME_cnt_us;

unsigned char key;

interrupt [TIM2_COMP] void timer2_cmp(void)
{
    T2_DUTY_TIME_cnt_us += 100;
    T2_CYCLE_TIME_cnt_us += 100;
    if(T2_DUTY_TIME_cnt_us <= T2_DUTY_TIME_us){SERVO_PIN_HIGH();}
    else{                                      SERVO_PIN_LOW();}
    
    if(T2_CYCLE_TIME_cnt_us == T2_CYCLE_TIME_us)
    {
        T2_CYCLE_TIME_cnt_us = 0;
        T2_DUTY_TIME_cnt_us = 0;
    }
}

void Init_Timer2(void)
{
    TIMSK |= (1<<OCIE2);
    TCCR2 = (1<< WGM21)|(2<<CS20);
    OCR2 = 184;
}

void Init_TimerINT(void)
{
    Init_Timer2();
    SREG |= 0x80;
}
    
void SetServoDeg(unsigned int deg)
{
    T2_DUTY_TIME_us = 500 + (deg*200/18);
}    


 
void ADC_Init(void)
{
    ADCSRA = 0x00;
    ADMUX = ADC_AVCC_TYPE | (0 << ADLAR) |(0 << MUX0);  
    ADCSRA = (1<<ADEN) | (1<<ADFR) |(3<<ADPS0);    
}

unsigned int Read_ADC_Data(unsigned char adc_input)
{
    unsigned int adc_Data = 0;
    
    ADMUX &= ~(0x1F);
    ADMUX |= (adc_input & 0x07);  
    
    ADCSRA |= (1<<ADSC);
    while(!(ADCSRA & (1<<ADIF)));
    adc_Data = ADCL;
    adc_Data |= ADCH<<8;
    
    return adc_Data;    
}

unsigned int Read_ADC_Data_Diff(unsigned char adc_mux)
{
    unsigned int ADC_Data = 0;
    
    if(adc_mux < 8)
        return 0xFFFF;
    
    ADMUX &= ~(0x1F);
    ADMUX |= (adc_mux & 0x1F);
    
    ADCSRA |= (1<<ADSC);
    
    while(!(ADCSRA & (1<<ADIF)));
    
    ADC_Data = ADCL;
    ADC_Data |= ADCH<<8;
    
    return ADC_Data;
}

void putch_USART1(char data)
 {
    while(!(UCSR1A&(1<<UDRE1)));
    UDR1 = data;
 }  
 
 void puts_USART1(char* str)
 {
    while(*str)
    {
        putch_USART1(*str);
        str++;
    }
 }
 
 unsigned char getch_USART1(void)
 {
    while(!(UCSR1A&(1<<RXC1)));
    return UDR1;    
 }    
 
 void Init_USART1(void)
 {
    UCSR1A = 0x00;   
    UCSR1B = (1<<RXEN1)|(1<<TXEN1);
    UCSR1C = (1<<UCSZ11)|(1<<UCSZ10);
    UCSR1C &= ~(1<<UMSEL1);
    
    UBRR1H = 0;
    UBRR1L = 15;
    
 }
 
 
void main(void)
 {
    int res = 0;
    int adcRaw = 0;
    float adcVoltage = 0;
    float light_Step;
    
    float adcmillivoltage = 0;
    char message[50];
    
    int ang=0;
    
    ADC_Init();
    LCD_Init();
    Init_USART1();
    
    LCD_Pos(0,0);
    LCD_Str("RawData:");
    
    LCD_Pos(1,0);    
    LCD_Str("R.Value:");  
                         
    DDRB = 0xff; 
     
    DDRD |= (1<<PORTD7);
    
    Init_TimerINT();
    
    T2_DUTY_TIME_us = 1500;
    T2_CYCLE_TIME_us = 20000;  
    
    while(1)
    {  
        key = (PIND & 0x0f);                         
        switch(key)
        {
            case 0x0e : 
                ang = 45;
                break;
            case 0x0d :
                  ang = 90; 
                  break;
            case 0x0b :
                ang = 120; 
                break;
            default:
                break;   
                }  

        if(ang>0)
        {
            while(1)
            {
    
                adcRaw = Read_ADC_Data(1);
                adcVoltage = ((((float)(adcRaw+1)*5)/1024));
        
                light_Step = -0.0354 * pow(adcVoltage,6) 
                             +0.5532 * pow(adcVoltage,5)
                             -3.3318 * pow(adcVoltage,4)
                             +9.7896 * pow(adcVoltage,3)
                             -14.712 * pow(adcVoltage,2)
                             +12*adcVoltage
                             -0.2201;
                     
                LCD_Pos(0,0);
                sprintf(message,"Raw: %d ,R.Value : %04d mV \r\n",adcRaw,(int)adcVoltage);
                puts_USART1(message); 
        
                LCD_Pos(0,0);
                sprintf(message,"RawData: %4d",adcRaw);
                LCD_Str(message);   
        
                LCD_Pos(1,0);
                sprintf(message,"%04d V, %02d",(int)adcVoltage*1000,(int)(light_Step*10+4)/10);
                LCD_Str(message); 
        
        
                if(adcRaw > 750){
                    SetServoDeg(ang);
                    delay_ms(1000);   
                }
                else if (adcRaw<600){ 
                SetServoDeg(0);
                delay_ms(1000);
                }       
            }    
        }  
    } 
 }
