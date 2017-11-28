
#include <SoftwareSerial.h>

#define DEBUG_MODE

#define PinLEDg         6
#define PinLEDr         7

//========================================================================== 
//             RS485         
//========================================================================== 
#define SSerialRX        10  //Serial Receive pin // RO
#define SSerialTX        8  //Serial Transmit pin // DI

#define SSerialTxControl1 9   //RS485 Direction control // DE

#define RS485Transmit    HIGH
#define RS485Receive     LOW

SoftwareSerial RS485Serial(SSerialRX, SSerialTX); // RX, TX

//========================================================================== 
//             RS485         
//========================================================================== 
//========================================================================== 
//             SWITCHER         
//========================================================================== 

#define PinSW0         A0
#define PinSW1         A1
#define PinSW2         A2
#define PinSW3         A3

long getSelectedBoud(){
  long BOUDS[8] =  {600, 1200, 2400, 4800, 9600, 19200, 38400, 57600};
  char bpos = 0;

  if (digitalRead(PinSW0) == HIGH){
    bpos += 1;
  }
  if (digitalRead(PinSW1) == HIGH){
    bpos += 2;
  }
  if (digitalRead(PinSW2) == HIGH){
    bpos += 4;
  }

  for (char i = 0; i < bpos+1; i++){
    digitalWrite(PinLEDg, LOW);
    delay(200);
    digitalWrite(PinLEDg, HIGH);    
    delay(200);
  }
  digitalWrite(PinLEDg, LOW);
  
  return BOUDS[bpos];
}

//========================================================================== 
//             SWITCHER         
//========================================================================== 

void setup() {
  // put your setup code here, to run once:

  // init PINS
  pinMode(PinLEDg, OUTPUT);   
  pinMode(PinLEDr, OUTPUT);   

  pinMode(PinSW0, INPUT);   
  pinMode(PinSW1, INPUT);   
  pinMode(PinSW2, INPUT);   
  pinMode(PinSW3, INPUT);   

  // RS485
  pinMode(SSerialTxControl1, OUTPUT);       
  RS485Serial.begin(getSelectedBoud());   // set the data rate

  // RS 232 COM PORT
  Serial.begin(getSelectedBoud());


}

//========================================================================== 
//             MAIN         
//========================================================================== 

void loop() {

  // put your main code here, to run repeatedly:
  if (RS485Serial.available()){
    
    digitalWrite(PinLEDg, HIGH);  // Show activity
    digitalWrite(PinLEDr, HIGH);  // Show activity
    while (RS485Serial.available()){ 
      Serial.write(RS485Serial.read());       
    }
    //delay(1);
    digitalWrite(PinLEDg, LOW); 
    digitalWrite(PinLEDr, LOW); 
    
    
     
  }

}

//========================================================================== 
//             MAIN         
//========================================================================== 
//========================================================================== 
//             UTILS         
//========================================================================== 

void serialEvent() {  
  digitalWrite(SSerialTxControl1, RS485Transmit);  // Enable RS485 Transmit 
  digitalWrite(PinLEDg, HIGH);  // Show activity
  while (Serial.available()) {
    RS485Serial.write(Serial.read());  
  }
  digitalWrite(PinLEDg, LOW);  
  digitalWrite(SSerialTxControl1, RS485Receive);  // Disable RS485 Transmit
}



