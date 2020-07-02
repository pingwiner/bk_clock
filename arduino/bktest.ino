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

#define CMD_SET_TIME  1
#define CMD_SET_DATE  2
#define CMD_GET_TIME  3
#define CMD_GET_DATE  4
#define CMD_GET_DATE_TIME_AS_STRING 5


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
byte cmd;
volatile bool receiveComplete;
byte recvBuf[64];

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


void onReceive() {
  receiveComplete = true;
  //Serial.println((char*) isrHandlerStruct.pBuffer);
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
  time.begin();
}

void onSetTime() {  
  receiveComplete = false;
  receiveBlock(recvBuf, 3, onReceive);
  while (!receiveComplete); 
  time.settime(recvBuf[2], recvBuf[1], recvBuf[0]);
  sendByte(0);  
}

void onGetDateTimeAsString() {  
  memset(recvBuf, 0, sizeof(recvBuf));
  receiveComplete = false;
  receiveString(recvBuf, sizeof(recvBuf) - 1, onReceive);  
  while (!receiveComplete); 
  sendString(time.gettime((char*)recvBuf));
}

void onSetDate() {
  receiveComplete = false;
  receiveBlock(recvBuf, 4, onReceive);
  while (!receiveComplete); 
  time.gettime();
  time.settime(time.seconds, time.minutes, time.hours, recvBuf[0], recvBuf[1], recvBuf[2], recvBuf[3]);
  sendByte(0);    
}

void onGetTime() {
  time.gettime();
  recvBuf[0] = time.hours;
  recvBuf[1] = time.minutes;
  recvBuf[2] = time.seconds;  
  sendBlock(recvBuf, 3);  
}

void onGetDate() {
  time.gettime();
  recvBuf[0] = time.day;
  recvBuf[1] = time.month;
  recvBuf[2] = time.year;  
  recvBuf[3] = time.weekday;  
  sendBlock(recvBuf, 4);    
}

void loop() {   
  cmd = receiveByte(); 
  switch(cmd) {
    case CMD_SET_TIME:
      onSetTime();      
      break;
    case CMD_GET_DATE_TIME_AS_STRING:
      onGetDateTimeAsString();
      break;
    case CMD_SET_DATE:
      onSetDate();
      break;  
    case CMD_GET_TIME:
      onGetTime();
      break;  
    case CMD_GET_DATE:
      onGetDate();
      break;    
    default:
      //unknown command  
      break;
  }  
  
}

byte receiveByte() {
  byte b;
  receiveComplete = false;
  receiveBlock(&b, 1, onReceive);
  while(!receiveComplete);
  return b;
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

void sendBlock(byte* p, int size) {
  do {
    sendByte(*p);
    p++;
    size--;
  } while (size != 0); 
}

void receiveBlock(void* buffer, int size, CallbackFunc callback) {
   isrInit();
   isrHandlerStruct.pBuffer = (byte*) buffer;
   isrHandlerStruct.bufferSize = size;   
   isrHandlerStruct.callback = callback;
   isrHandlerStruct.isrState = ISR_STATE_BUSY;
}

void receiveString(void* buffer, int size, CallbackFunc callback) {
   isrInit();
   isrHandlerStruct.pBuffer = (byte*) buffer;
   isrHandlerStruct.bufferSize = size;   
   isrHandlerStruct.callback = callback;
   isrHandlerStruct.mode = ISR_MODE_RECEIVE_STRING;
   isrHandlerStruct.isrState = ISR_STATE_BUSY;  
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
