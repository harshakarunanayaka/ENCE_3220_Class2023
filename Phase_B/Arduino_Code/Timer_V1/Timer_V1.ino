#define BUTTON_2  2  // Inc Timer
#define BUTTON_1  3  // Start/Stop Timer and Stop Buzzer
#define GREEN_LED 4
#define RED_LED   5
#define BUZZER    6

#define DATA      9 // DS
#define LATCH     8 // ST_CP
#define CLOCK     7 // SH_CP

#define DIGIT_4   10
#define DIGIT_3   11
#define DIGIT_2   12
#define DIGIT_1   13

#define BUFF_SIZE 20
#define DEFAULT_COUNT 30     // default value is 30secs

// 7-Seg Display Variables
unsigned char gtable[]=
{0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c
,0x39,0x5e,0x79,0x71,0x00};
byte gCurrentDigit;

// Volatile Variables
volatile unsigned char gISRFlag1   = 0;
volatile unsigned char gBuzzerFlag = 0;
volatile unsigned char gISRFlag2   = 0;

// Timer Variables
volatile int  gCount        = DEFAULT_COUNT;
unsigned char gTimerRunning = 0; 

unsigned int gReloadTimer1 = 62500; // corresponds to 1 second
byte         gReloadTimer3 = 10;  // display refresh time
unsigned int gReloadTimer4 = 100;   // corresponds to 0.4ms

// Serial Data Reading Variables
char  gIncomingChar;
char  gCommsMsgBuff[BUFF_SIZE];
int   iBuff = 0;
byte  gPackageFlag = 0;
byte  gProcessDataFlag = 0;
byte gTimerStopFlag = 0; // Flag for stop timer
byte gTimerStartFlag = 0; // FLag for start timer
byte gTimerSetFlag = 0; // Flag for set timer
int gSetCount = 0; // Variable for set count by the web applictaion


/**
 * @brief Setup peripherals and timers
 * @param
 * @return
 */
void setup() {

  // Initialize Serial Communicatin
  Serial.begin(9600);
  
  // LEDs Pins
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  // LEDs -> Timer Stopped
  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, HIGH);

  // Button Pins
  pinMode(BUTTON_1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_1), buttonISR1, RISING);
  pinMode(BUTTON_2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_2), buttonISR2, RISING);

  // Buzer Pins
  pinMode(BUZZER, OUTPUT);

  // Buzzer -> Off
  digitalWrite(BUZZER,LOW);

  // 7-Seg Display
  pinMode(DIGIT_1, OUTPUT);
  pinMode(DIGIT_2, OUTPUT);
  pinMode(DIGIT_3, OUTPUT);
  pinMode(DIGIT_4, OUTPUT);

  // Shift Register Pins
  pinMode(LATCH, OUTPUT);
  pinMode(CLOCK, OUTPUT);
  pinMode(DATA, OUTPUT);

  dispOff();  // turn off the display

  // Initialize Timer1 (16bit) -> Used for clock
  // Speed of Timer1 = 16MHz/256 = 62.5 KHz
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  OCR1A = gReloadTimer1; // compare match register 16MHz/256
  TCCR1B |= (1<<WGM12);   // CTC mode
  // Start Timer by setting the prescaler -> done using the start button
  //TCCR1B |= (1<<CS12);    // 256 prescaler 
  TIMSK1 |= (1<<OCIE1A);  // enable timer compare interrupt
  interrupts();

  // Initialize Timer3 (16bit) -> Used to refresh display
  // Speed of Timer2 = 16MHz/1024 = 15.625 KHz
  TCCR3A = 0;
  TCCR3B = 0;
  OCR3A = gReloadTimer3;                     // max value 2^8 - 1 = 255
  TCCR3A |= (1<<WGM31);
  TCCR3B = (1<<CS32)| (1<<CS30); // 1024 prescaler
  TIMSK3 |= (1<<OCIE3A);
  interrupts();                               // enable all interrupts

  // Initialize Timer4 (10bit)-> Used to read the serial data every 0.4ms
  // Speed of Timer4 = 64MHz/256 = 250KHz
  TCCR4A = 0;
  TCCR4B = 0;
  OCR4A = gReloadTimer4;                     
  TCCR4A |= (1<<WGM41);
  TCCR4B = (1<<CS43) | (1<<CS40); // 256 prescaler
  TIMSK4 |= (1<<OCIE4A);
  interrupts();                               // enable all interrupts
}

/**
 * @brief Compare two arrays
 * @param a
 * @param b
 * @return result
 */
char compareArray(char a[], char b[], int size)
{
  int i;
  char result = 1;  // default: the arrays are equal
  
  for(i = 0; i<size; i++)
  {
    if(a[i]!=b[i])
    {
      result = 0;
      break;
    }
  }
  return result;
}

/**
 * @brief Shifts the bits through the shift register
 * @param num
 * @param dp
 * @return
 */
void display(unsigned char num, unsigned char dp)
{
  digitalWrite(LATCH, LOW);
  shiftOut(DATA, CLOCK, MSBFIRST, gtable[num] | (dp<<7));
  digitalWrite(LATCH, HIGH);
}

/**
 * @brief Turns the 7-seg display off
 * @param
 * @return
 */
void dispOff()
{
   digitalWrite(DIGIT_1, HIGH);
   digitalWrite(DIGIT_2, HIGH);
   digitalWrite(DIGIT_3, HIGH);
   digitalWrite(DIGIT_4, HIGH);
}

/**
 * @brief Button 2 ISR
 * @param
 * @return
 */
void buttonISR2()
{
  // Increment Clock
  gCount++;
}

/**
 * @brief Button 1 ISR
 * @param
 * @return
 */
void buttonISR1()
{ 
  // Set ISR Flag
  gISRFlag1 = 1;
}

/**
 * @brief Timer 2 ISR
 * @param TIMER2_COMPA_vect
 * @return
 */
ISR(TIMER3_COMPA_vect)   // Timer2 interrupt service routine (ISR)
{
  dispOff();  // turn off the display
 
  switch (gCurrentDigit)
  {
    case 1: //0x:xx
      display( int((gCount/60) / 10) % 6, 0 );   // prepare to display digit 1 (most left)
      digitalWrite(DIGIT_1, LOW);  // turn on digit 1
      break;
 
    case 2: //x0:xx
      display( int(gCount / 60) % 10, 1 );   // prepare to display digit 2
      digitalWrite(DIGIT_2, LOW);     // turn on digit 2
      break;
 
    case 3: //xx:0x
      display( (gCount / 10) % 6, 0 );   // prepare to display digit 3
      digitalWrite(DIGIT_3, LOW);    // turn on digit 3
      break;
 
    case 4: //xx:x0
      display(gCount % 10, 0); // prepare to display digit 4 (most right)
      digitalWrite(DIGIT_4, LOW);  // turn on digit 4
      break;

    default:
      break;
  }
 
  gCurrentDigit = (gCurrentDigit % 4) + 1;
}

/**
 * @brief Timer 1 ISR
 * @param TIMER1_COMPA_vect
 * @return
 */
ISR(TIMER1_COMPA_vect)  // Timer1 interrupt service routine (ISR)
{
  gCount--;

  if(gCount == 0)
  {
      // Stop Timer
      stopTimer1();
      
      // Raise Alarm
      gBuzzerFlag = 1;
      gTimerRunning = 0;
  }
}

/**
 * @brief Timer 1 ISR
 * @param TIMER1_COMPA_vect
 * @return
 */
ISR(TIMER4_COMPA_vect)  // Timer4 interrupt service routine (ISR)
{
  if(Serial.available()>0)
  {
    gISRFlag2 = 1;
  }
}

/**
 * @brief Stop Timer 1
 * @param
 * @return
 */
void stopTimer1()
{
  // Stop Timer
  TCCR1B &= 0b11111000; // stop clock
  TIMSK1 = 0; // cancel clock timer interrupt
}

/**
 * @brief Start Timer 1
 * @param
 * @return
 */
void startTimer1()
{
  // Start Timer
  TCCR1B |= (1<<CS12);    // 256 prescaler 
  TIMSK1 |= (1<<OCIE1A);  // enable timer compare interrupt
}

/**
 * @brief Turn On Buzzer
 * @param
 * @return
 */
void activeBuzzer()
{
  unsigned char i;
  unsigned char sleepTime = 1; // ms
  
  for(i=0;i<100;i++)
  {
    digitalWrite(BUZZER,HIGH);
    delay(sleepTime);//wait for 1ms
    digitalWrite(BUZZER,LOW);
    delay(sleepTime);//wait for 1ms
  }
}

/**
 * @brief Read the aerial data
 * @param
 * @return
 */
void serialDataRead()
{
      // Read serial
    gIncomingChar = Serial.read();

    // If normal character from package
    if(gPackageFlag == 1)
    {
      gCommsMsgBuff[iBuff] = gIncomingChar;
      iBuff++;

      // Safety mechanism in case "\n" is never sent
      if(iBuff == BUFF_SIZE)
      {
        gPackageFlag = 0;
        gProcessDataFlag = 1;
      }
    }

    // If start of the package
    if(gIncomingChar == '$')
    {    
      gPackageFlag = 1;  // Signal start of package
      
      // Clear Buffer
      for(int i=0; i<BUFF_SIZE; i++)
      {
        gCommsMsgBuff[i] = 0;
      }

      // set gCommsMsgBuff Index to zero
      iBuff = 0;
    }

    // If end of package
    if( (gIncomingChar == '\n') && (gPackageFlag == 1) )
    {
      // Signal end of package
      gPackageFlag = 0;
      gProcessDataFlag = 1;
    }
  

  // Process serial commands
  if(gProcessDataFlag == 1)
  {
    gProcessDataFlag = 0;
    
    if(compareArray(gCommsMsgBuff, "STR", 3) == 1)
    {
      // Start timer function
      gTimerStartFlag = 1;
    }
  
    if(compareArray(gCommsMsgBuff, "STP", 3) == 1)
    {
      // Stop timer function
      gTimerStopFlag =1;
    }

    if(compareArray(gCommsMsgBuff, "INC", 3) == 1)
    {
      // increment the timer
       gCount++;
    }

    if(compareArray(gCommsMsgBuff, "GET", 3) == 1)
    {
      // Send clock status
      //x0:00
      char m1 = int((gCount/60) / 10) % 6;
      //0x:00
      char m2 = int(gCount / 60) % 10;
      //00:x0
      char s1 = int(gCount / 10) % 6;
      //00:0x
      char s2 = int(gCount % 10);

      // Send timer digits to web application
      Serial.print("$");
      Serial.print(m1);
      Serial.print(m2);
      Serial.print(":");
      Serial.print(s1);
      Serial.print(s2);
      Serial.print("/n");
    }
    if(compareArray(gCommsMsgBuff, "SET,TMRS,",9) == 1)
    {
      gTimerSetFlag=1;
      // convert respective characters to integervlaues
      int tempM1 = char(gCommsMsgBuff[strlen(gCommsMsgBuff)-4])-'0'; //x0:00
      int tempM2 = char(gCommsMsgBuff[strlen(gCommsMsgBuff)-3])-'0';//0x:00
      int tempS1 = char(gCommsMsgBuff[strlen(gCommsMsgBuff)-1])-'0';//00:x0
      int tempS2 = char(gCommsMsgBuff[strlen(gCommsMsgBuff)]-'0');//00:0x
      //Calculate gCount accordingly for respective time set value

      gSetCount = int(((tempM1%6)*10)*60)+int((tempM2%10)*60)+int((tempS1%6)*10)+int(tempS2%10);
      
    }
  }
}
/**
 * @brief Main Loop
 * @param
 * @return
 */
void loop() 
{
  // Attend Timer4 flag - receive commands through serialcommunication
  if(gISRFlag2 == 1)
  {    
    // Reset ISR Flag
    gISRFlag2 = 0;
    serialDataRead();
    
  }
  
  // Attend button 2 ISR,Timer Start and Stop commands from Web application
  if((gISRFlag1 == 1)||(gTimerStartFlag==1)||(gTimerStopFlag==1))
  {
    // Reset ISR Flag
    gISRFlag1 = 0;

    if(gTimerRunning == 0)
    {
      // Start Timer
      gTimerRunning = 1;
      
      if((gCount == 0)&&(gTimerSetFlag==1))
      {
        gTimerSetFlag = 0;
        gCount = gSetCount;
      }
      else
      {
        gCount = DEFAULT_COUNT;
      }

      if(gBuzzerFlag == 1)
      {
        gBuzzerFlag = 0;

        // LEDs -> Timer Stopped
        digitalWrite(RED_LED, HIGH);
        digitalWrite(GREEN_LED, HIGH);
      }
      else
      {
        startTimer1();
        // LEDs -> Timer Running
        digitalWrite(RED_LED, LOW);
        digitalWrite(GREEN_LED, HIGH);
      }
    }
    else
    {
      // Stop Timer
      stopTimer1();
      gTimerRunning = 0;

      // LEDs -> Timer Running
      digitalWrite(RED_LED, HIGH);
      digitalWrite(GREEN_LED, HIGH);
    }
  }

  // Attend gBuzzerFlag
  if(gBuzzerFlag == 1)
  {
    // Make Noise...
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    activeBuzzer();
  }
}
