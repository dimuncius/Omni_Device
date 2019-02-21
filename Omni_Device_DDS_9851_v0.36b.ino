
//***************************************************** Libraries ******************************************************
#include "TimerOne.h"               // Timer1 lib
#include "MsTimer2.h"               // Timer 2 lib
#include <DallasTemperature.h>      // DS18B20 Libs
#include <OneWire.h>                
#include <Wire.h>  					// OneWire Lib
#include <LiquidCrystal_I2C.h>      // I2C LCD Lib
LiquidCrystal_I2C lcd(0x27,16,2);   

//***************************************************** Variables and constants ***********************************************
#define Interrupt_ENCODER_Pin 3                // Encoder pin A (Interrupt) on Pin 3
#define Reset_ENCODER_Pin     9                // Reset encoder triggers on Pin 9
#define Button_ENC_Pin        10               // Encoder button on Pin 10
#define Button_ADD_Pin        12               // Side button on Pin 12
#define ENCODER_B_Pin         11               // Encoder pin B on Pin 11
#define T_ResetENCODER        70               // Time for encoder to be reset after reading
#define ONE_WIRE_BUS_Pin      4                // Pin 4 for receiving data from DS18B20
#define BUTTON_DELAY          220 
#define DDS_CLOCK 180000000

float temp0, temp1;                            // Temperature variables
int my_Chart_1 = 0;                            // Custom symbol
byte Hrad[8] ={                                // Bitmap for custom Celsius cymbol
              B00110,
              B01001,
              B01001,
              B00110,
              B00000,
              B00000,
              B00000,
              B00000};                  



#include "RTClib.h"
RTC_DS1307 rtc;
#define DS1307_I2C_ADDRESS 0x68

int v = 0;
int a = 0;
int k = 0;

byte LOAD = 7; 
byte CLOCK = 6; 
byte DATA = 5;
byte LED = 13;
byte RESET = 8;

int MENU = 0;
int MENU_OLD = 0;
boolean MENU_CHANGE= true; // Unlocking menu scrolling
int STATE = 1 ;
boolean STEP_COUNTER_STATE;

int INDEX_OLD=6;
int INDEX=1;
unsigned long FREQ_STEP = pow(10,INDEX)+1;
unsigned long FREQ=50;
int Clock_Position;

boolean Generator_ENABLED = false; // Generator status
boolean Clock_ENABLED = false;     // Clock status
boolean Set_Clock_ENABLED = false; // Clock settings status

int In_Hour;                       // Temporary time variables
int In_Min;
int In_Sec;
int In_Weekday;
int In_Day;
int In_Month;
int In_Year;

OneWire oneWire(ONE_WIRE_BUS_Pin);             // Starting OneWire for connecting DS18B20 
DallasTemperature sensors(&oneWire);           // OneWire device is Dallas Temperature sensor

unsigned long Count =  0;                                // Encoder counter variables
unsigned long Count_old = 0;                              

//::::::::::::::::::::::::::::::::::::::::::::::: Variables and constants for interrupts:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
volatile boolean Right_ENC = false;            // Encoder rotates right
volatile boolean Left_ENC = false;             // Encoder rotates left
volatile boolean Button_ENC = false;           // Encoder button is pressed
volatile boolean Button_ADD = false;           // Side button is pressed
volatile boolean Ready_ENCODER = true;         // Encoder is ready for being read
volatile boolean Button_ADD_2 = false;
volatile boolean Button_ADD_3 = false;
volatile boolean First = false;

//=============================================== Interrupt functions ======================================== 

void ENCODER_Interupt()          // Encoder interrupts
                          
{
  if (Ready_ENCODER)
    {
      if ( digitalRead ( ENCODER_B_Pin ) )                                        // Check rotation right
      {
        Right_ENC = true;
        Left_ENC = false;  
      }
      else                                                                        // Check rotation left
      {                                                           
        Right_ENC = false;
        Left_ENC = true;  
      }
      if ( digitalRead ( Button_ENC_Pin ) ) Button_ENC = true;          // Check if encoder button is pressed
      else Button_ENC = false;   
      MENU_COUNTER();
      FREQ_COUNTER();
      STEP_COUNTER();
      CLOCK_COUNTER();
      Ready_ENCODER = false;                                                 
    }
} 
 
void Reset_ENCODER()                // Resets encoder status on Timer2
                            
    {
        digitalWrite(Reset_ENCODER_Pin, HIGH); 
        Ready_ENCODER = true;                         
        digitalWrite(Reset_ENCODER_Pin, LOW);       
    }
    
//************************************************************** Initialization***************************************************
void setup() 
{ 
  Serial.begin(115200);           //  Debug serial connection   

  lcd.init();                                                       // Initialize LCD
  lcd.backlight();
  lcd.home();

  rtc.begin();                         // Start RTC
  lcd.createChar(my_Chart_1,Hrad);      // Create custom symbol
  
  pinMode(13, OUTPUT);
  
  pinMode(Interrupt_ENCODER_Pin, INPUT_PULLUP);                     
  digitalWrite(Interrupt_ENCODER_Pin, HIGH);                        

  pinMode(Button_ADD_Pin, INPUT_PULLUP);                            
  digitalWrite(Button_ADD_Pin, HIGH);                              

  pinMode(Button_ENC_Pin, INPUT_PULLUP);                           
  digitalWrite(Button_ENC_Pin, HIGH);                              

  pinMode(ENCODER_B_Pin, INPUT_PULLUP);                            
  digitalWrite(ENCODER_B_Pin, HIGH);                               
  
  pinMode(Reset_ENCODER_Pin, OUTPUT);                              
  digitalWrite(Reset_ENCODER_Pin, LOW);   

  Ready_ENCODER = false;
              
  attachInterrupt(1, ENCODER_Interupt, RISING);                     // Attaching interrupt on rising front on Pin3
  ENCODER_Reset();                                                  
  MsTimer2::set(T_ResetENCODER, Reset_ENCODER);                     // Initialyze Timer2
  MsTimer2::start();                                                // Start Timer2
  sensors.begin();    

  pinMode (RESET, OUTPUT); 
  pinMode (DATA, OUTPUT); // sets pin 10 as OUPUT
  pinMode (CLOCK, OUTPUT); // sets pin 9 as OUTPUT
  pinMode (LOAD, OUTPUT); // sets pin 8 as OUTPUT
  pinMode (LED, OUTPUT);
  Reset_DDS();
  Serial_INIT();
  Generator_ENABLED = false;
  MENU_CHANGE=true;
  MENU = 0;
  MENU_CHANGER();   
} 

void loop() //Main cycle
{  
  if (!Ready_ENCODER)  decision_Making_ENC();      // Process encoder data if ready          
  Button_READ();
  if( Button_ADD  )                              // Process side button
  {
    if(Set_Clock_ENABLED) Clock_Position++; // Switching positions in clock setup menu
    if ((!Generator_ENABLED)&&(!Clock_ENABLED)&&(!Set_Clock_ENABLED)&&(MENU==0)) // Enable generator
    {
      Generator_ENABLED=true; 
      STATE =1;
    }                  
    if ((!Generator_ENABLED)&&(!Clock_ENABLED)&&(!Set_Clock_ENABLED)&&(MENU==1)) // Enable clock
    {
      Clock_ENABLED=true;  
      lcd.setCursor(0,0);
      lcd.print("                ");
      lcd.setCursor(0,1); 
      lcd.print("                ");   
    }
    else //disable clock
    {                  
      Clock_ENABLED= false;
    }
    if ((!Generator_ENABLED)&&(!Clock_ENABLED)&&(!Set_Clock_ENABLED)&&(MENU==2)) // Enable clock setup
    {
      Set_Clock_ENABLED=true;  
      lcd.setCursor(0,0);
      lcd.print("                ");
      lcd.setCursor(0,1); 
      lcd.print("                ");   
      get_time(); // Store current time in temporary variables
    }                
    Button_ADD=false;                        
    Serial.println(Button_ADD);                                                            
  }
  MENU_CHANGER(); // Switch menu screen
  if (Generator_ENABLED) Generator(); 
  if(Clock_ENABLED) Clock();    
  if(Set_Clock_ENABLED) CLOCK_CHANGER();                                                                                                                                                           
}

//***************************************************** Common functions ******************************************************


void ENCODER_Reset()                            //  Reset encoder 
                                                                                                          
{
  digitalWrite(Reset_ENCODER_Pin, LOW);           
  digitalWrite(Reset_ENCODER_Pin, HIGH);
  Ready_ENCODER = true;
  digitalWrite(Reset_ENCODER_Pin, LOW);
  return;
}

void decision_Making_ENC ()   // Make decision to process encoder end reset buttons
                              
{
  ENCODER_Reset(); 
  return;
}

void outOne() // Put 1 to sda
{
  digitalWrite(CLOCK, LOW);
  digitalWrite(DATA, HIGH);
  digitalWrite(CLOCK, HIGH);
  digitalWrite(DATA, LOW);
}

void outZero() // Put 0 to sda
{
  digitalWrite(CLOCK, LOW);
  digitalWrite(DATA, LOW);
  digitalWrite(CLOCK, HIGH);
}

void Reset_DDS() // Reset Generator
{
  digitalWrite(RESET, LOW);
  digitalWrite(RESET, HIGH);
  digitalWrite(RESET, LOW);  
}

void sendFrequency(unsigned long frequency, unsigned char byte) // Setup generator
{
  unsigned long tuning_word = (frequency * pow(2, 32)) / DDS_CLOCK;
  digitalWrite (LOAD, LOW); // take load pin low
  for(int i = 0; i < 32; i++)
  {
    if ((tuning_word & 1) == 1)
    outOne();
    else
    outZero();
    tuning_word = tuning_word >> 1;
  }
  for (int i = 0; i < 8; i++)
  {
    if ((byte & 1) == 1)
    outOne();
    else
    outZero();
    byte = byte >> 1;
  }
  digitalWrite (LOAD, HIGH); // Take load pin high again
}

void Serial_INIT() // Initialize serial port
{
  digitalWrite(CLOCK, LOW);
  digitalWrite(CLOCK, HIGH);
  digitalWrite(CLOCK, LOW);
  digitalWrite(LOAD, LOW);
  digitalWrite(LOAD, HIGH);
  digitalWrite(LOAD, LOW); 
}

void Button_READ()// Read button
{
  Button_ADD_2=digitalRead(Button_ADD_Pin);
  if(Button_ADD_2)
    {
      delay(BUTTON_DELAY);
      Button_ADD_2=digitalRead(Button_ADD_Pin);
      if (!Button_ADD_2) Button_ADD=true;  
    }
  else Button_ADD=false;    
}

void Generator() // Generator function
{ 
Button_READ();
if ((Ready_ENCODER)&&(Count!=Count_old))
  {
    lcd.setCursor(6,0);
    lcd.print("             ");
    lcd.setCursor(6,0); 
    FREQ=Count;
    lcd.print(Count);     
    lcd.setCursor(14,0);
    lcd.print("Hz");                 
    Count_old=Count;
    if((STATE==3)&&(!Button_ADD))                                               // Change frequency
    {                      
      Serial_INIT();                           
      sendFrequency(FREQ, 0x0D);
      Serial_INIT();                           
      sendFrequency(FREQ, 0x09);
    }
  }   

   if ((Ready_ENCODER)&&(INDEX!=INDEX_OLD))
  {
    lcd.setCursor(6,1);
    lcd.print("             ");
    lcd.setCursor(6,1); 
    if (INDEX>1)
    FREQ_STEP=pow(10,INDEX)+1;
    else FREQ_STEP=pow(10,INDEX);
    lcd.print(FREQ_STEP);     
    lcd.setCursor(14,1);
    lcd.print("Hz");                 
    INDEX_OLD=INDEX;
  }
  
if ((Generator_ENABLED)&&(Button_ADD))    
  switch(STATE)
  {
    case 0: { STATE+=1;  break; }
    case 1: { STATE+=1;  break; }
    case 2: { STATE-=1;  break; }
    case 3: { STATE-=3;  break; }
  }                  
if ((Generator_ENABLED)&&(!Button_ADD))
  switch(STATE)
  {
    case 0:                                               // Disable generator
    { 
      Serial_INIT();                           
      sendFrequency(FREQ, 0x0D);
      Serial.println("0D");
      lcd.setCursor(0,0);
      lcd.print("                 ");
      lcd.setCursor(0,1);
      lcd.print("                 ");
      lcd.setCursor(2,1); 
      lcd.print("STOPPING");
      STATE +=2 ;                    
      Generator_ENABLED=false;                   
      MENU_CHANGE=true;
      delay(2000);
      break;
    }
    case 1:                                               // Enable generator
    {                                              
      Serial_INIT();
      FREQ = 50;
      sendFrequency(FREQ, 0x09);
      Serial.println("0x09");
      lcd.setCursor(0,0);
      lcd.print("                ");
      lcd.setCursor(0,0);
      lcd.print("Freq:");
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print("Step:");
      lcd.setCursor(6,1);
      lcd.print(FREQ_STEP);
      lcd.setCursor(14,1);
      lcd.print("Hz");                                     
      STATE +=2;                    
      break;
    }
    case 2:                                
      break;                   
    case 3:                                               
      break;                      
  }  
}

void STEP_COUNTER() 
{
  if((Generator_ENABLED)&&(Button_ENC))
  { 
    STEP_COUNTER_STATE=true; 
    if (Right_ENC) INDEX++;                                           // Counts steps for frequency
    if (Left_ENC) INDEX--;
    if(INDEX>=7) INDEX--;
    if(INDEX<0) INDEX=0;
  }
  else STEP_COUNTER_STATE=false;      
}

void FREQ_COUNTER()
{
  if((Generator_ENABLED)&&(!STEP_COUNTER_STATE))
  {  
    if (Right_ENC) Count+=FREQ_STEP;                                           // Counts frequency
    if (Left_ENC) Count-=FREQ_STEP;
    if(Count>=76000000) Count-=FREQ_STEP;
    if(Count<=1) Count+=FREQ_STEP;
  }
}

void MENU_COUNTER() // Counts menu screen
{
  if ((!Generator_ENABLED)&&(!Clock_ENABLED))
  {
    if (Right_ENC) MENU++;
    if (Left_ENC) MENU--;         
    if ( MENU == -1) MENU = 4;
    if ( MENU == 5 ) MENU = 0;
    MENU_CHANGE=true;
  }
}

void CLOCK_COUNTER() // Counts clock setup position
{
  if ((!Generator_ENABLED)&&(!Clock_ENABLED)&&(Set_Clock_ENABLED))
  {
    MENU_CHANGE=false;        
    switch(Clock_Position)
    {
      case 5: 
      { 
        if(Right_ENC) //Change hours
        {
          In_Hour++; 
          if(In_Hour>=23) In_Hour=23;
          if(In_Hour<=0) In_Hour=0;               
        }
        if(Left_ENC)  
        {
          In_Hour--; 
          if(In_Hour>=23) In_Hour=23;
          if(In_Hour<=0) In_Hour=0;              
        }
        break;
      }
      case 6: //minutes
      {             
        if(Right_ENC) 
        {
          In_Min++;
          if(In_Min>=59) In_Min=59;
          if(In_Min<=0) In_Min=0; 
        } 
        if(Left_ENC) 
        {
          In_Min--;            
          if(In_Min>=59) In_Min=59;
          if(In_Min<=0) In_Min=0;                
        } 
        break;
      }
      case 7: //seconds
      {             
        if(Right_ENC) 
        {
          In_Sec++;
          if(In_Sec>=59) In_Sec=59;
          if(In_Sec<=0) In_Sec=0;  
        }
        if(Left_ENC)
        {
          In_Sec--;
          if(In_Sec>=59) In_Sec=59;
          if(In_Sec<=0) In_Sec=0;                
         }
         break;
       }
       case 4: //exit setup
       {
         if((Right_ENC)||(Left_ENC))
         {
           Clock_Position = 0;
           Set_Clock_ENABLED=false;
           MENU_CHANGE=true;  
         }
         break;
       }
       case 1://maximum days
       {            
         if(Right_ENC) 
         {
           In_Day++;
           if((In_Month==1)||(In_Month==3)||(In_Month==5)||(In_Month==7)||(In_Month==8)||(In_Month==10)||(In_Month==12))
            if(In_Day>=31) In_Day=31;
           if((In_Month==4)||(In_Month==6)||(In_Month==9)||(In_Month==11))
            if(In_Day>=30) In_Day=30;  
           if(( In_Year%4==0 ) &&(In_Month==2))
            if(In_Day>=29) In_Day=29;
           if(( In_Year%4!=0 ) &&(In_Month==2))
            if(In_Day>=29) In_Day=29;
           if(In_Day<=1) In_Day=1;               
         }
         if(Left_ENC) 
         {
          In_Day--;
          if((In_Month==1)||(In_Month==3)||(In_Month==5)||(In_Month==7)||(In_Month==8)||(In_Month==10)||(In_Month==12))
            if(In_Day>=31) In_Day=31;
          if((In_Month==4)||(In_Month==6)||(In_Month==9)||(In_Month==11))
            if(In_Day>=30) In_Day=30;  
          if(( In_Year%4==0 ) &&(In_Month==2))
            if(In_Day>=29) In_Day=29;
          if(( In_Year%4!=0 ) &&(In_Month==2))
            if(In_Day>=29) In_Day=29;
          if(In_Day<=1) In_Day=1;                
        }            
        break;
      }
      case 0: //day of week
      {             
        if(Right_ENC) 
        {
          In_Weekday++;
          if(In_Weekday>=7) In_Weekday=7;
          if(In_Weekday<=1) In_Weekday=1;             
        }
        if(Left_ENC) 
        {
          In_Weekday--;
          if(In_Weekday>=7) In_Weekday=7;
          if(In_Weekday<=1) In_Weekday=1;            
        }
        break;
      }
      case 2://month
      {            
        if(Right_ENC) 
        {
          In_Month++;
          if(In_Month>=12) In_Month=12;
          if(In_Month<=1) In_Month=1;               
        }
        if(Left_ENC) 
        {
          In_Month--;
          if(In_Month>=12) In_Month=12;
          if(In_Month<=1) In_Month=1;                
        }            
        break;
      }
      case 3: //year
      {            
        if(Right_ENC) 
        {
          In_Year++;
          if(In_Year<=2000) In_Year=2000;                
        }
        if(Left_ENC) 
        {
          In_Year--;
          if(In_Year<=2000) In_Year=2000;               
        } 
        break;
      }
      case 8: //exit setup
      {
        if((Right_ENC)||(Left_ENC))
        {
          Clock_Position = 0;
          Set_Clock_ENABLED=false;  
          MENU_CHANGE=true;
        }
        break;
      }                                     
    }       
  }
}

void CLOCK_CHANGER() // Visualize clock setup
{    
  switch(Clock_Position)//Select variable
  {
    case 5:  //hours
    { 
      lcd.setCursor(0,1);
      lcd.print(In_Hour);
      delay(200);
      lcd.setCursor(0,1);
      lcd.print("  ");
      delay(80);
      lcd.setCursor(2,1);
      lcd.print(":");
      lcd.setCursor(4,1);
      lcd.print(In_Min);
      lcd.setCursor(6,1);
      lcd.print(":");
      lcd.setCursor(8,1);
      lcd.print(In_Sec);
      lcd.setCursor(14,1);
      lcd.print("OK");
      lcd.setCursor(0,0);
      switch(In_Weekday) //Print weekday
      {
        case 1: { lcd.print("Mo"); break;}
        case 2: { lcd.print("Tu"); break;}
        case 3: { lcd.print("We"); break;}
        case 4: { lcd.print("Th"); break;}
        case 5: { lcd.print("Fr"); break;}
        case 6: { lcd.print("Sa"); break;}
        case 7: { lcd.print("Su"); break;}
      }
      lcd.setCursor(2,0);
      lcd.print(" ");
      lcd.setCursor(3,0);
      lcd.print(In_Day);
      lcd.setCursor(5,0);
      lcd.print(".");
      lcd.setCursor(6,0);
      lcd.print(In_Month);
      lcd.setCursor(8,0);
      lcd.print(".");     
      lcd.setCursor(9,0);
      lcd.print(In_Year);
      lcd.setCursor(13,0);
      lcd.print(" ");
      lcd.setCursor(14,0);
      lcd.print("OK");  
      break;
    }
    case 6: // minutes
    {               
      lcd.setCursor(0,1);
      lcd.print(In_Hour);
      lcd.setCursor(2,1);
      lcd.print(":");
      lcd.setCursor(4,1);
      lcd.print(In_Min);
      delay(200);
      lcd.setCursor(4,1);
      lcd.print("  ");
      delay(80);
      lcd.setCursor(6,1);
      lcd.print(":");
      lcd.setCursor(8,1);
      lcd.print(In_Sec);
      lcd.setCursor(14,1);
      lcd.print("OK");  
      break;
    }
    case 7: //secounds
    { 
      lcd.setCursor(0,1);
      lcd.print(In_Hour);
      lcd.setCursor(2,1);
      lcd.print(":");
      lcd.setCursor(4,1);
      lcd.print(In_Min);
      lcd.setCursor(6,1);
      lcd.print(":");
      lcd.setCursor(8,1);
      lcd.print(In_Sec);
      delay(200);
      lcd.setCursor(8,1);
      lcd.print("  ");
      delay(80);
      lcd.setCursor(14,1);
      lcd.print("OK");       
      break;
    }
    case 8: // ОК
    {            
      lcd.setCursor(0,1);
      lcd.print(In_Hour);
      lcd.setCursor(2,1);
      lcd.print(":");
      lcd.setCursor(4,1);
      lcd.print(In_Min);
      lcd.setCursor(6,1);
      lcd.print(":");
      lcd.setCursor(8,1);
      lcd.print(In_Sec);
      lcd.setCursor(14,1);
      lcd.print("OK");  
      delay(200);
      lcd.setCursor(14,1);
      lcd.print("  ");
      delay(80);  
      break;
    }
    case 0: //weekdays
    { 
      lcd.setCursor(0,1);
      lcd.print(In_Hour);
      lcd.setCursor(2,1);
      lcd.print(":");
      lcd.setCursor(4,1);
      lcd.print(In_Min);
      lcd.setCursor(6,1);
      lcd.print(":");
      lcd.setCursor(8,1);
      lcd.print(In_Sec);
      lcd.setCursor(14,1);
      lcd.print("OK");
      lcd.setCursor(0,0);
      lcd.print("  ");                        
      delay(80);
      lcd.setCursor(0,0);
      switch(In_Weekday)
      {
        case 1: { lcd.print("Mo"); break;}
        case 2: { lcd.print("Tu"); break;}
        case 3: { lcd.print("We"); break;}
        case 4: { lcd.print("Th"); break;}
        case 5: { lcd.print("Fr"); break;}
        case 6: { lcd.print("Sa"); break;}
        case 7: { lcd.print("Su"); break;}
      }
      delay(200);
      lcd.setCursor(2,0);
      lcd.print(" ");
      lcd.setCursor(3,0);
      lcd.print(In_Day);
      lcd.setCursor(5,0);
      lcd.print(".");
      lcd.setCursor(6,0);
      lcd.print(In_Month);
      lcd.setCursor(8,0);
      lcd.print(".");     
      lcd.setCursor(9,0);
      lcd.print(In_Year);
      lcd.setCursor(13,0);
      lcd.print(" ");
      lcd.setCursor(14,0);
      lcd.print("OK");        
      break;
    }
    case 1: //days
    {      
      lcd.setCursor(0,0);      
      switch(In_Weekday)
      {
        case 1: { lcd.print("Mo"); break;}
        case 2: { lcd.print("Tu"); break;}
        case 3: { lcd.print("We"); break;}
        case 4: { lcd.print("Th"); break;}
        case 5: { lcd.print("Fr"); break;}
        case 6: { lcd.print("Sa"); break;}
        case 7: { lcd.print("Su"); break;}
      }           
      lcd.setCursor(2,0);
      lcd.print(" ");
      lcd.setCursor(3,0);
      lcd.print(In_Day);
      delay(200);
      lcd.setCursor(3,0);
      lcd.print("  ");
      delay(80);
      lcd.setCursor(5,0);
      lcd.print(".");
      lcd.setCursor(6,0);
      lcd.print(In_Month);
      lcd.setCursor(8,0);
      lcd.print(".");     
      lcd.setCursor(9,0);
      lcd.print(In_Year);
      lcd.setCursor(13,0);
      lcd.print(" ");
      lcd.setCursor(14,0);
      lcd.print("OK");                    
      break;
    }
    case 2: // month
    {   
      lcd.setCursor(0,0);         
      switch(In_Weekday)
      {
        case 1: { lcd.print("Mo"); break;}
        case 2: { lcd.print("Tu"); break;}
        case 3: { lcd.print("We"); break;}
        case 4: { lcd.print("Th"); break;}
        case 5: { lcd.print("Fr"); break;}
        case 6: { lcd.print("Sa"); break;}
        case 7: { lcd.print("Su"); break;}
      }           
      lcd.setCursor(2,0);
      lcd.print(" ");
      lcd.setCursor(3,0);
      lcd.print(In_Day);            
      lcd.setCursor(5,0);
      lcd.print(".");
      lcd.setCursor(6,0);
      lcd.print(In_Month);
      delay(200);
      lcd.setCursor(6,0);
      lcd.print("  ");
      delay(80);
      lcd.setCursor(8,0);
      lcd.print(".");     
      lcd.setCursor(9,0);
      lcd.print(In_Year);
      lcd.setCursor(13,0);
      lcd.print(" ");
      lcd.setCursor(14,0);
      lcd.print("OK");               
      break;
    }
    case 3: //year
    {        
      lcd.setCursor(0,0);    
      switch(In_Weekday)
      {
        case 1: { lcd.print("Mo"); break;}
        case 2: { lcd.print("Tu"); break;}
        case 3: { lcd.print("We"); break;}
        case 4: { lcd.print("Th"); break;}
        case 5: { lcd.print("Fr"); break;}
        case 6: { lcd.print("Sa"); break;}
        case 7: { lcd.print("Su"); break;}
      }           
      lcd.setCursor(2,0);
      lcd.print(" ");
      lcd.setCursor(3,0);
      lcd.print(In_Day);            
      lcd.setCursor(5,0);
      lcd.print(".");
      lcd.setCursor(6,0);
      lcd.print(In_Month);            
      lcd.setCursor(8,0);
      lcd.print(".");  
      lcd.setCursor(9,0);
      lcd.print("    ");
      delay(80);   
      lcd.setCursor(9,0);
      lcd.print(In_Year);
      delay(200);            
      lcd.setCursor(13,0);
      lcd.print(" ");
      lcd.setCursor(14,0);
      lcd.print("OK");               
      break;
    }
    case 4://ОК
    {
      lcd.setCursor(0,0);
      switch(In_Weekday)
      {
        case 1: { lcd.print("Mo"); break;}
        case 2: { lcd.print("Tu"); break;}
        case 3: { lcd.print("We"); break;}
        case 4: { lcd.print("Th"); break;}
        case 5: { lcd.print("Fr"); break;}
        case 6: { lcd.print("Sa"); break;}
        case 7: { lcd.print("Su"); break;}
      }        
      lcd.setCursor(2,0);
      lcd.print(" ");
      lcd.setCursor(3,0);
      lcd.print(In_Day);            
      lcd.setCursor(5,0);
      lcd.print(".");
      lcd.setCursor(6,0);
      lcd.print(In_Month);            
      lcd.setCursor(8,0);
      lcd.print(".");     
      lcd.setCursor(9,0);
      lcd.print(In_Year);
      lcd.setCursor(13,0);
      lcd.print(" ");
      lcd.setCursor(14,0);
      lcd.print("OK");
      delay(200);
      lcd.setCursor(14,0);
      lcd.print("  ");
      delay(80);    
      break;       
    }
    case 9: //save settings and exit to menu
    {
     //setDateDs1307(In_Sec,In_Min,In_Hour,In_Weekday,In_Day,In_Month,In_Year);
      Clock_Position = 0;
      Set_Clock_ENABLED=false;
      MENU_CHANGE=true;  
      break;
    }
  }        
}               

void MENU_CHANGER() // Menu visualization
{
  if((Ready_ENCODER)&&(MENU!=MENU_OLD) &&(!Generator_ENABLED))
    MENU_OLD=MENU;    
  if(MENU_CHANGE)                        
    switch(MENU)
    {
      case 0:
      {                       
        lcd.setCursor(0,0);
        lcd.print("                ");
        lcd.setCursor(0,1); 
        lcd.print("                ");
        MENU_CHANGE=false;                            
        lcd.setCursor(0,0);                    
        lcd.print("> Generator");
        lcd.setCursor(0,1); 
        lcd.print("  Clock");  
        MENU_CHANGE=false;                    
        break;               
      }
      case 1:
      {                           
        lcd.setCursor(0,0);
        lcd.print("                ");
        lcd.setCursor(0,1); 
        lcd.print("                "); 
        MENU_CHANGE=false;                                       
        lcd.setCursor(0,0);
        lcd.print("> Clock");
        lcd.setCursor(0,1); 
        lcd.print("  Set Clock");  
        MENU_CHANGE=false;
        break;               
      }
      case 2:
      {                       
        lcd.setCursor(0,0);
        lcd.print("                ");
        lcd.setCursor(0,1); 
        lcd.print("                "); 
        MENU_CHANGE=false;                                              
        lcd.setCursor(0,0);
        lcd.print("> Set Clock");
        lcd.setCursor(0,1); 
        lcd.print("  Thermometer");  
        MENU_CHANGE=false;
        break;               
      }
      case 3:
      {                     
        lcd.setCursor(0,0);
        lcd.print("                ");
        lcd.setCursor(0,1); 
        lcd.print("                ");   
        MENU_CHANGE=false;                                               
        lcd.setCursor(0,0);
        lcd.print("> Thermometer");
        lcd.setCursor(0,1); 
        lcd.print("  Herzmeter");    
        MENU_CHANGE=false;
        break;             
      }
      case 4:
      {                       
        lcd.setCursor(0,0);
        lcd.print("                ");
        lcd.setCursor(0,1); 
        lcd.print("                ");   
        MENU_CHANGE=false;                                               
        lcd.setCursor(0,0);
        lcd.print("> Herzmeter");
        lcd.setCursor(0,1); 
        lcd.print("  Generator");   
        MENU_CHANGE=false;    
        break;          
      }
    } 
}

byte setDateDs1307(
    byte second,        // 0-59
    byte minute,        // 0-59
    byte hour,          // 1-23
    byte dayOfWeek,     // 1-7
    byte dayOfMonth,    // 1-28/29/30/31
    byte month,         // 1-12
    byte year
  )          // 0-99
{
   Wire.beginTransmission(DS1307_I2C_ADDRESS);
   Wire.write(0);
   Wire.write(decToBcd(second));    
   Wire.write(decToBcd(minute));
   Wire.write(decToBcd(hour));     
   Wire.write(decToBcd(dayOfWeek));
   Wire.write(decToBcd(dayOfMonth));
   Wire.write(decToBcd(month));
   Wire.write(decToBcd(year));
   Wire.endTransmission();
}

byte decToBcd(byte v) // Dec to bcd convertation
{  
  return ( (v/10*16) + (v%10) );
}

byte bcdToDec(byte v) // Bcd to dec convertation
{   
  return ( (v/16*10) + (v%16) );
}

void setDateDs1307() // Load settings to clock
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(byte(0));
  Wire.write(decToBcd(In_Sec));
  Wire.write(decToBcd(In_Min));
  Wire.write(decToBcd(In_Hour));
  Wire.write(decToBcd(In_Weekday));
  Wire.write(decToBcd(In_Day));
  Wire.write(decToBcd(In_Month));
  Wire.write(decToBcd(In_Year));
  Wire.write(byte(0));
  Wire.endTransmission();
}
void getDateDs1307(byte *second, byte *minute, byte *hour, byte *dayOfWeek, byte *dayOfMonth,   byte *month,   byte *year)/*get current date and time*/
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();
  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
  *second     = bcdToDec(Wire.read() & 0x7f);
  *minute     = bcdToDec(Wire.read());
  *hour       = bcdToDec(Wire.read() & 0x3f); 
  *dayOfWeek  = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month      = bcdToDec(Wire.read());
  *year       = bcdToDec(Wire.read());
}

void get_time() // set current date and time to temporary variables
{
  Wire.beginTransmission(DS1307_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();
  Wire.requestFrom(DS1307_I2C_ADDRESS, 7);
  In_Sec = bcdToDec(Wire.read() & 0x7f);
  In_Min     = bcdToDec(Wire.read());
  In_Hour       = bcdToDec(Wire.read() & 0x3f); 
  In_Weekday  = bcdToDec(Wire.read());
  In_Day = bcdToDec(Wire.read());
  In_Month      = bcdToDec(Wire.read());
  In_Year       = bcdToDec(Wire.read())+1952;
}

void printDigits(int digits){               // prints 0 before hours    
   if(digits < 10)
   {
    lcd.print('0');
   }
   lcd.print(digits);
}

void Out_Clock()
{                        
  DateTime now = rtc.now();
  lcd.setCursor(0, 0);                      
  lcd.print(now.hour());
  lcd.print(":");
  printDigits(now.minute());
  lcd.print(":");
  printDigits(now.second());   
  //День, месяц и год
  lcd.setCursor(9, 0);                   
  lcd.print(now.day());
  lcd.print("/");
  lcd.print(now.month());
  lcd.print("/");
  lcd.print(now.year()-2048);
}
 
void Clock() // Clock screen function
{
  if(Clock_ENABLED)
  {  
    if (Button_ADD) { Clock_ENABLED = false;   MENU_CHANGE=true;}
    MENU_CHANGE=false;   
    sensors.requestTemperatures();                    
    temp0= sensors.getTempCByIndex(1);                
    lcd.setCursor(2, 1);                           
    lcd.print(temp1+0.12);                         
    lcd.write(my_Chart_1);                         
    temp1= sensors.getTempCByIndex(0);                
    lcd.setCursor(10, 1);                           
    if ( temp0 == -127 )  lcd.print("t1=OFF"); 
    else 
    { 
      lcd.print(temp0+0.12);        
      lcd.write(my_Chart_1);
    }   
                                     
    byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;  
    getDateDs1307(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month, &year);   
    Out_Clock();                      
  }
}

