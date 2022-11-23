#include "SerialTransfer.h"

enum __attribute__((__packed__)) ACTION{
  UP,
  DOWN,
  LEFT,
  RIGHT,
  STOP,
  REQ
} act;

float __attribute((packed)) val = 0;

SerialTransfer st;

int cw1 = 2, cw2 = 4;
int ccw1 = 3, ccw2 = 5;
void setup(){
  Serial.begin(9600);
  Serial1.begin(76800);
  st.begin(Serial1);
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
  pinMode(cw1,OUTPUT);
  pinMode(cw2,OUTPUT);
  pinMode(ccw1,OUTPUT);
  pinMode(ccw2,OUTPUT);
  Serial2.begin(9600);
}

void front(){
  digitalWrite(ccw1,HIGH);
  digitalWrite(ccw2,HIGH);
}

void back(){
  digitalWrite(cw1,HIGH);
  digitalWrite(cw2,HIGH);
}

void right(){
  digitalWrite(ccw1,HIGH);
}

void left(){
  digitalWrite(ccw2,HIGH);
}

void stop(){
  digitalWrite(cw1,LOW);
  digitalWrite(cw2,LOW);
  digitalWrite(ccw1,LOW);
  digitalWrite(ccw2,LOW);
}

void loop() {
  while(Serial2.available()){
    Serial.write(Serial2.read());
  }    
  if(st.available()){
    st.rxObj(act);
    if(act == UP){
      front();
    }else if(act == DOWN){
      back();
    }else if(act == LEFT){
      left();
    }else if(act == RIGHT){
      right();
    }else if(act == STOP){
      stop();
    }else if(act == REQ){
        float x = (float) 5 / 1024 * analogRead(A0);
        val = (float) 7 + ((2.5 - x) / 0.167) + 3.55;
        st.sendDatum(val);
    }
  }

}