
#include <SoftwareSerial.h>
#include "toemreader232w.h"


#define DEBUG_MODE

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
//             WIEGAND   SETTINGS      
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
//             UTILS         
//========================================================================== 
void sendBufToRs485(uint8_t* buff, uint8_t bufflen){
    digitalWrite(SSerialTxControl1, RS485Transmit);  // Enable RS485 Transmit 
    for (uint8_t i = 0; i < bufflen; i++){
      RS485Serial.write(buff[i]);          // Send byte to Remote Arduino  
    }
    delay(10);
    digitalWrite(SSerialTxControl1, RS485Receive);  // Disable RS485 Transmit        
}
//==========================================================================
void sendBufToRs232(uint8_t* buff, uint8_t bufflen){
    for (uint8_t i = 0; i < bufflen; i++){
      Serial.write(buff[i]);          // Send byte to Remote Arduino  
    }
}
//========================================================================== 
void blink_ok (){
  digitalWrite(PinGreedLED, LOW);
  delay(150);
  digitalWrite(PinGreedLED, HIGH);
  delay(150);
  digitalWrite(PinGreedLED, LOW);
  delay(150);
}
//==========================================================================
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
//========================================================================== 
//             MAIN   PART      
//==========================================================================
void printWiegandData(){
  unsigned int lv = 0;

  Serial.print("Data: ");
  
  for (uint8_t i = 0; i < bitCount; i++)
  {
    Serial.print(databits[i]);
    
    if ((i+1) % 8 == 0)
      Serial.print(" ");

    if (databits[bitCount-i-1]){
      lv |= 1 << i;  
    }
  }

  Serial.print(" Long: ");
  Serial.print(lv);

  Serial.print(" Read ");
  Serial.print(bitCount);
  Serial.print(" bits. ");
}
//==========================================================================
void workWiegand26(){
  // standard 26 bit format
  // facility code = bits 2 to 9
  for (uint8_t i=2; i<=9; i++){
     facilityCode <<=1;
     facilityCode |= databits[i];
  }
  // card code = bits 10 to 23
  for (uint8_t i=9; i<25; i++){
     cardCode <<=1;
     cardCode |= databits[i];
  }
  sendCode();  
}
//==========================================================================
void workWiegand32(){
  for (uint8_t i=0; i<=15; i++){
     facilityCode <<=1;
     facilityCode |= databits[i];
  }
  for (uint8_t i=16; i<=31; i++){
     cardCode <<=1;
     cardCode |= databits[i];
  }
  sendCode();  
}
//==========================================================================
void workWiegand37(){
  // 37 bit HID Corporate H10304 format
  // facility code = bits 1 to 16
  for (uint8_t i=1; i<=16; i++){
     facilityCode <<=1;
     facilityCode |= databits[i];
  }
  // card code = bits 15 to 35 ???
  for (uint8_t i=15; i<=35; i++){
     cardCode <<=1;
     cardCode |= databits[i];
  }
  sendCode(); 
}
//==========================================================================
void workWiegandUndecode(){
  undecodeable_code = 0;
  for (uint8_t i = 0; i < bitCount; i++){
    if (databits[bitCount-i-1]){
      undecodeable_code |= (uint64_t)1 << i;
    }
  }
}
//==========================================================================
//    MAIN LOOP LOOP LOOP LOOP LOOP
//==========================================================================
void loop() {

  digitalWrite(PinGreedLED, HIGH);  // Show activity

  // This waits to make sure that there have been no more data pulses before processing data
  if (!flagDone) {
    if (--weigand_counter == 0)
      flagDone = 1;  
  }
 
  // if we have bits and we the weigand counter went out
  if (bitCount > 0 && flagDone) {

    #ifdef DEBUG_MODE
    printWiegandData();
    #endif

    switch (bitCount){
      case 37:
        workWiegand37();
      break;
      case 32:
        workWiegand32();
      break;
      case 26:
        workWiegand26();
      break;
      default:
        workWiegandUndecode();
      break;
    }

    // cleanup and get ready for the next card
    bitCount = 0;
    facilityCode = 0;
    cardCode = 0;
    for (uint8_t i=0; i<MAX_BITS; i++) 
    {
     databits[i] = 0;
    }
  }
}
//===============================================================================
void sendCode()
{
  // I really hope you can figure out what this function does
  uint8_t buff[25];
  uint8_t l = g_toemreader232w_p->buildMessage(buff, 25, 0x73, facilityCode, cardCode);

  #ifdef DEBUG_MODE
  Serial.print("FC = ");
  Serial.print(facilityCode);
  Serial.print(", CC = ");
  Serial.print(cardCode); 
  
  Serial.print(" >> ");
  for (uint8_t i = 0; i < l; i++){
    Serial.write(buff[i]);
  }
  Serial.println("");
  #endif

  #ifndef DEBUG_MODE
  sendBufToRs232(buff, l);
  sendBufToRs485(buff, l);
  #endif
  
  blink_ok();
}
//===============================================================================
//         O T H E R   U T I L S
//===============================================================================
/*
void printLong64Bits(uint64_t data)
{
      sendLong64ToRs485(data);
      
      #ifndef DEBUG_MODE
      sendLong64ToRs232(data);
      #endif

      blink_ok();
}
*/
//===============================================================================
/*
void printLong32Bits(uint32_t data)
{
      sendLongToRs485(data);
      
      #ifndef DEBUG_MODE
      sendLongToRs232(data);
      #endif

      blink_ok();
}
*/
//===============================================================================
/*
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
*/
//===============================================================================
/*
void sendLong64ToRs485(uint64_t val){
  long * pl;
  pl = (long*)&val;
  sendLongToRs485(*pl);
  pl++;
  sendLongToRs485(*pl);
}
*/
//==========================================================================
/*
void sendLongToRs232(long val)
{
    for (char i = 0; i < 4; i++){
      char * c = (char *)&val + i;
      Serial.write(*c);          // Send byte to Remote Arduino  
    }

    //digitalWrite(PinGreedLED, LOW);  // Show activity    
    delay(10);
}
*/
//===============================================================================
/*
void sendLong64ToRs232(uint64_t val){
  long * pl;
  pl = (long*)&val;
  sendLongToRs232(*pl);
  pl++;
  sendLongToRs232(*pl);
}
*/
//===============================================================================

