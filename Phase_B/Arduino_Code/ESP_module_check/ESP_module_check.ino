/////////////////////////////////////////////////////////////////////////////////////////////
/////////////This code read the serial msg received over wifi and if message is 'STR"////////
//////////// it switch on the LED and if the message is "STP" it switch off the LED./////////

#define BUFF_SIZE 20
char gIncChar;
char gMsgBuffer[BUFF_SIZE];
int iBuff = 0;
//byte gPackageFLag = 0;
//byte gDataProcessFlag = 0;

char compareArray (char a[],char b[], int size)
{
  char output =1; // 1 for if arrays are matching, and 0 otherwise

  for (int i=0;i<size;i++)
  {
    if (a[i]!=b[i])
    {
      output = 0; 
      break;
    }
  }
  return output; 
}


void setup() {
  
  Serial.begin(9600); // initialize serial communication
  pinMode(LED_BUILTIN,OUTPUT); // Initialize built-in LED
  
}

void loop() {

  if (Serial.available() > 0) 
  {
    char gIncChar = Serial.read();
    
    if (gIncChar == "$")
    { 
      while(gIncChar=="/n")
      {
        gMsgBuffer[iBuff] = gIncChar;
        iBuff++;
      }
      iBuff=0;
    }

     if(compareArray(gMsgBuffer,"STR",3)==1)
     {
      digitalWrite(LED_BUILTIN,HIGH);
     }
     if(compreArray(gMsgBuffer,"STP",3)==1)
     {
      digitalWrite(LED_BUILTIN,LOW);
     }
  }

}
