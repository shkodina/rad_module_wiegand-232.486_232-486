
#include <SoftwareSerial.h>

#define DEBUG_MODE
#define PARSE_WIEGAND_37
#define PARSE_WIEGAND_26

//========================================================================== 
//             RS485         
//========================================================================== 
#define SSerialRX        10  //Serial Receive pin // RO
#define SSerialTX        8  //Serial Transmit pin // DI

#define SSerialTxControl1 9   //RS485 Direction control // DE

#define RS485Transmit    HIGH
#define RS485Receive     LOW

#define PinGreedLED         6
#define PinRedLED         7

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

uint64_t undecodeable_code=0;        // if cannot decode will send this

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

  pinMode(PinGreedLED, OUTPUT);   
  pinMode(PinRedLED, OUTPUT);   

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

    //digitalWrite(Pin13LED, LOW);  // Show activity    
    delay(10);
    digitalWrite(SSerialTxControl1, RS485Receive);  // Disable RS485 Transmit        
}

void sendLong64ToRs485(long val){
  long * pl;
  pl = (long*)&val;
  sendLongToRs485(*pl);
  pl++;
  sendLongToRs485(*pl);
}
//==========================================================================

void sendLongToRs232(long val)
{
    for (char i = 0; i < 4; i++){
      char * c = (char *)&val + i;
      Serial.write(*c);          // Send byte to Remote Arduino  
    }

    //digitalWrite(PinGreedLED, LOW);  // Show activity    
    delay(10);
}

void sendLong64ToRs232(uint64_t val){
  long * pl;
  pl = (long*)&val;
  sendLongToRs232(*pl);
  pl++;
  sendLongToRs232(*pl);
}

//========================================================================== 
//             MAIN         
//========================================================================== 

void blink_ok (){
  digitalWrite(PinGreedLED, LOW);
  delay(150);
  digitalWrite(PinGreedLED, HIGH);
  delay(150);
  digitalWrite(PinGreedLED, LOW);
  delay(150);
}

void blink_error (){
  digitalWrite(PinGreedLED, LOW);
  digitalWrite(PinRedLED, HIGH);
  delay(150);
  digitalWrite(PinRedLED, LOW);
  delay(150);
  digitalWrite(PinRedLED, HIGH);
  delay(150);
  digitalWrite(PinRedLED, LOW);
}

void loop() {

  //long x = 305441741;
  digitalWrite(PinGreedLED, HIGH);  // Show activity
  // This waits to make sure that there have been no more data pulses before processing data
  if (!flagDone) {
    if (--weigand_counter == 0)
      flagDone = 1;  
  }
 
  // if we have bits and we the weigand counter went out
  if (bitCount > 0 && flagDone) {
    unsigned char i;

    unsigned int lv = 0;

    undecodeable_code = 0;

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

      if (databits[bitCount-i-1]){
        lv |= 1 << i;  
        undecodeable_code |= (uint64_t)1 << i;
      }
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
    
    if (bitCount == 37)
    {
      #ifdef PARSE_WIEGAND_37
      
      // 37 bit HID Corporate H10304 format
      // facility code = bits 1 to 16
      for (i=4; i<=19; i++)
      {
         facilityCode <<=1;
         facilityCode |= databits[i];
      }
 
      // card code = bits 15 to 35 ???
      for (i=20; i<=35; i++)
      {
         cardCode <<=1;
         cardCode |= databits[i];
      }
 
      printBits();

      #endif // PARSE_WIEGAND_37
      
      #ifndef PARSE_WIEGAND_37
        printLong64Bits(undecodeable_code);
      #endif
      
    }
    else if (bitCount == 26)
    {
      #ifdef PARSE_WIEGAND_26
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

      #endif // PARSE_WIEGAND_26
        
      #ifndef PARSE_WIEGAND_26
        printLong32Bits(lv);
      #endif
      
    }
    else {
      // you can add other formats if you want!
      #ifdef DEBUG_MODE
      Serial.println("Unable to decode."); 
      #endif
      blink_error ();
      
      sendLong64ToRs485(undecodeable_code);
      
      #ifndef DEBUG_MODE
      sendLong64ToRs232(undecodeable_code);
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

      blink_ok();
}

void printLong64Bits(uint64_t data)
{
      sendLong64ToRs485(data);
      
      #ifndef DEBUG_MODE
      sendLong64ToRs232(data);
      #endif

      blink_ok();
}

void printLong32Bits(uint32_t data)
{
      sendLongToRs485(data);
      
      #ifndef DEBUG_MODE
      sendLongToRs232(data);
      #endif

      blink_ok();
}
