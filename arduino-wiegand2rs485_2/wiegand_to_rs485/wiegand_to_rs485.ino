
#include <SoftwareSerial.h>

//#define DEBUG_MODE

//========================================================================== 
//             RS485         
//========================================================================== 
#define SSerialRX        10  //Serial Receive pin // RO
#define SSerialTX        8  //Serial Transmit pin // DI

#define SSerialTxControl1 9   //RS485 Direction control // DE

#define RS485Transmit    HIGH
#define RS485Receive     LOW

#define Pin13LED         6
#define Pin7LED         7

SoftwareSerial RS485Serial(SSerialRX, SSerialTX); // RX, TX

//========================================================================== 
//             RS485         
//========================================================================== 

//========================================================================== 
//             WIEGAND         
//========================================================================== 

#define MAX_BITS 100                 // max number of bits 
#define WEIGAND_WAIT_TIME  3000      // time to wait for another weigand pulse.  
 
unsigned char databits[MAX_BITS];    // stores all of the data bits
unsigned char bitCount;              // number of bits currently captured
unsigned char flagDone;              // goes low when data is currently being captured
unsigned int weigand_counter;        // countdown until we assume there are no more bits
 
unsigned long facilityCode=0;        // decoded facility code
unsigned long cardCode=0;            // decoded card code

const byte interruptPin_green = 2;   // DO
const byte interruptPin_white = 3;   // D1 
volatile byte state = LOW;

//========================================================================== 
//             WIEGAND         
//========================================================================== 

// interrupt that happens when INTO goes low (0 bit)
void ISR_INT0()
{
  #ifdef DEBUG_MODE
  Serial.print("0");   // uncomment this line to display raw binary
  #endif
  
  bitCount++;
  flagDone = 0;
  weigand_counter = WEIGAND_WAIT_TIME;  
 
}
 
// interrupt that happens when INT1 goes low (1 bit)
void ISR_INT1()
{
  #ifdef DEBUG_MODE
  Serial.print("1");   // uncomment this line to display raw binary
  #endif
  
  databits[bitCount] = 1;
  bitCount++;
  flagDone = 0;
  weigand_counter = WEIGAND_WAIT_TIME;  
}

//========================================================================== 
//             SETUP         
//========================================================================== 
void setup() {

  pinMode(Pin13LED, OUTPUT);   
  pinMode(Pin7LED, OUTPUT);   

  // RS485
  pinMode(SSerialTxControl1, OUTPUT);       
  RS485Serial.begin(9600);   // set the data rate

  // WIEGAND
  pinMode(interruptPin_green, INPUT_PULLUP);
  pinMode(interruptPin_white, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin_green), ISR_INT0, FALLING);
  attachInterrupt(digitalPinToInterrupt(interruptPin_white), ISR_INT1, FALLING);
  weigand_counter = WEIGAND_WAIT_TIME;

  // RS 232 COM PORT
  Serial.begin(9600);

  #ifdef DEBUG_MODE
  Serial.println("RFID Readers");
  #endif

 
}

//========================================================================== 
//             RS 485 UTILS         
//========================================================================== 

void sendLongToRs485(long val)
{
    digitalWrite(SSerialTxControl1, RS485Transmit);  // Enable RS485 Transmit 
 
    for (char i = 0; i < 4; i++){
      char * c = (char *)&val + i;
      RS485Serial.write(*c);          // Send byte to Remote Arduino  
    }

    digitalWrite(Pin13LED, LOW);  // Show activity    
    delay(10);
    digitalWrite(SSerialTxControl1, RS485Receive);  // Disable RS485 Transmit        
}

//==========================================================================

void sendLongToRs232(long val)
{
    for (char i = 0; i < 4; i++){
      char * c = (char *)&val + i;
      Serial.write(*c);          // Send byte to Remote Arduino  
    }

    digitalWrite(Pin7LED, LOW);  // Show activity    
    delay(10);
}


//========================================================================== 
//             MAIN         
//========================================================================== 

void loop() {

  //long x = 305441741;
  digitalWrite(Pin13LED, HIGH);  // Show activity
  digitalWrite(Pin7LED, HIGH);  // Show activity
  // This waits to make sure that there have been no more data pulses before processing data
  if (!flagDone) {
    if (--weigand_counter == 0)
      flagDone = 1;  
  }
 
  // if we have bits and we the weigand counter went out
  if (bitCount > 0 && flagDone) {
    unsigned char i;

    unsigned int lv = 0;

    #ifdef DEBUG_MODE
    Serial.print("Data: ");
    #endif
    
    for (i = 0; i < bitCount; i++)
    {
      #ifdef DEBUG_MODE
      Serial.print(databits[i]);
      
      if ((i+1) % 8 == 0)
        Serial.print(" ");

      #endif  

      if (databits[bitCount-i-1])
        lv |= 1 << i;  
    }

    #ifdef DEBUG_MODE
    Serial.print(" Long: ");
    Serial.print(lv);
    #endif

    //sendLongToRs485(lv);

    #ifdef DEBUG_MODE
    Serial.print(" Read ");
    Serial.print(bitCount);
    Serial.print(" bits. ");
    #endif
 
    // we will decode the bits differently depending on how many bits we have
    // see www.pagemac.com/azure/data_formats.php for mor info
    
    if (bitCount == 35)
    {
      // 35 bit HID Corporate 1000 format
      // facility code = bits 2 to 14
      for (i=2; i<14; i++)
      {
         facilityCode <<=1;
         facilityCode |= databits[i];
      }
 
      // card code = bits 15 to 34
      for (i=14; i<34; i++)
      {
         cardCode <<=1;
         cardCode |= databits[i];
      }
 
      printBits();
    }
    else if (bitCount == 26)
    {
      // standard 26 bit format
      // facility code = bits 2 to 9
      for (i=1; i<9; i++)
      {
         facilityCode <<=1;
         facilityCode |= databits[i];
      }
 
      // card code = bits 10 to 23
      for (i=9; i<25; i++)
      {
         cardCode <<=1;
         cardCode |= databits[i];
      }
 
      printBits();  
    }
    else {
      // you can add other formats if you want!
      #ifdef DEBUG_MODE
      Serial.println("Unable to decode."); 
      #endif
    }
 
     // cleanup and get ready for the next card
     bitCount = 0;
     facilityCode = 0;
     cardCode = 0;
     for (i=0; i<MAX_BITS; i++) 
     {
       databits[i] = 0;
     }
  }
}

void printBits()
{
      // I really hope you can figure out what this function does
      #ifdef DEBUG_MODE
      Serial.print("FC = ");
      Serial.print(facilityCode);
      Serial.print(", CC = ");
      Serial.println(cardCode); 
      #endif
      
      sendLongToRs485(cardCode);
      
      #ifndef DEBUG_MODE
      sendLongToRs232(cardCode);
      #endif
}
