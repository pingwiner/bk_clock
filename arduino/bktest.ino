#include <iarduino_RTC.h>

const byte interruptPin = 3;

#define ISR_STATE_IDLE  0
#define ISR_STATE_BUSY  1
#define ISR_STATE_READY 2

#define ISR_NIBBLE1     0
#define ISR_NIBBLE2     1
#define ISR_BYTE_READY  2

#define ISR_MODE_RECEIVE_BLOCK  0
#define ISR_MODE_RECEIVE_STRING 1

typedef void (*CallbackFunc)(); 

typedef struct {
  byte dataState;
  byte data;
  byte* pBuffer;
  int bufferSize;
  int currentPosition;
  byte isrState;
  CallbackFunc callback;
  byte mode;
} IsrHandlerStruct;

volatile IsrHandlerStruct isrHandlerStruct;
char recvBuf[64];

void isrInit() {
  isrHandlerStruct.dataState = ISR_NIBBLE1;
  isrHandlerStruct.data = 0;
  isrHandlerStruct.pBuffer = nullptr;
  isrHandlerStruct.bufferSize = 0;
  isrHandlerStruct.currentPosition = 0;
  isrHandlerStruct.isrState = ISR_STATE_IDLE;
  isrHandlerStruct.callback = nullptr;
  isrHandlerStruct.mode = ISR_MODE_RECEIVE_BLOCK;
}

void receiveBlock(byte* buffer, int size, CallbackFunc callback) {
   isrInit();
   isrHandlerStruct.pBuffer = buffer;
   isrHandlerStruct.bufferSize = size;   
   isrHandlerStruct.callback = callback;
   isrHandlerStruct.isrState = ISR_STATE_BUSY;
}

void receiveString(char* buffer, int size, CallbackFunc callback) {
   isrInit();
   isrHandlerStruct.pBuffer = (byte*) buffer;
   isrHandlerStruct.bufferSize = size;   
   isrHandlerStruct.callback = callback;
   isrHandlerStruct.mode = ISR_MODE_RECEIVE_STRING;
   isrHandlerStruct.isrState = ISR_STATE_BUSY;  
}

void onReceive() {
  Serial.print("data received: ");
  Serial.println((char*) isrHandlerStruct.pBuffer);
}

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
  isrInit();
  attachInterrupt(digitalPinToInterrupt(interruptPin), isr, CHANGE);
  //Serial.println("waiting for data ...");
  
  time.begin();
  //Serial.println(time.gettime("d-m-Y, H:i:s, D"));
  //receiveString(recvBuf, sizeof(recvBuf), onReceive);
}

void loop() {  
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
  if (isrHandlerStruct.isrState != ISR_STATE_BUSY) return;
  bool shouldStop = false;
  
  switch(isrHandlerStruct.dataState) {
    case ISR_NIBBLE1:
      isrHandlerStruct.data = ((~PIND) >> 4) & 15;      
      isrHandlerStruct.dataState++;
      break;
    case ISR_NIBBLE2:
      isrHandlerStruct.data |= ((~PIND) & 0xf0);      
      isrHandlerStruct.dataState++;
      isrHandlerStruct.pBuffer[isrHandlerStruct.currentPosition++] = isrHandlerStruct.data;
      
      if (isrHandlerStruct.mode == ISR_MODE_RECEIVE_STRING) {
        if (isrHandlerStruct.data == 0) {
          shouldStop = true;
        }
      }
      if ((isrHandlerStruct.currentPosition >= isrHandlerStruct.bufferSize) || shouldStop) {
        isrHandlerStruct.isrState = ISR_STATE_READY;
        if (isrHandlerStruct.callback != nullptr) {
          isrHandlerStruct.callback();
        }
      } else {
        isrHandlerStruct.data = 0;
        isrHandlerStruct.dataState = ISR_NIBBLE1;
      }
      break;
    default:  
      break;
  }
}
