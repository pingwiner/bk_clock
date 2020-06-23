#include <iarduino_RTC.h>

const byte interruptPin = 3;
volatile byte dataState = 0;
volatile byte data = 0;
iarduino_RTC time(RTC_DS1302, 8, 10, 9);

void setup() {
  Serial.begin(9600);
  pinMode(7, INPUT);
  pinMode(6, INPUT);
  pinMode(5, INPUT);
  pinMode(4, INPUT);
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), isr, CHANGE);
  //Serial.println("waiting for data ...");
  
  time.begin();
  //Serial.println(time.gettime("d-m-Y, H:i:s, D"));
}

void loop() {  
  //if (dataState != 2) return;
  //Serial.println(data);
  dataState = 0;
  delay(1000);
  sendString(time.gettime("H:i:s"));    
}

void sendByte(byte a) {
  byte temp = PORTC;
  temp &= 0xf0;
  temp |= (a & 0xf); 
  PORTC = temp;
  digitalWrite(A4, LOW);
  delay(1);
  temp = PORTC;
  temp &= 0xf0;
  temp |= (a >> 4);       
  PORTC = temp;
  digitalWrite(A4, HIGH);
  delay(1);
}

void sendString(char* p) {
  while(1) {
    sendByte(*p);
    if (*p == 0) break;
    p++;
  } 
}

void isr() {
  switch(dataState) {
    case 0:
      data = ((~PIND) >> 4) & 15;      
      dataState++;
      break;
    case 1:
      data |= ((~PIND) & 0xf0);      
      dataState++;
      break;
    default:  
      break;
  }
}
