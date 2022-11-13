#include "SerialTransfer.h"

enum __attribute__((__packed__)) ACTION{
  UP,
  DOWN,
  LEFT,
  RIGHT,
  STOP
} act;

SerialTransfer st;

void setup(){
  Serial.begin(115200);
  Serial1.begin(76800);
  st.begin(Serial1);
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
}

int cur = 0;
void loop() {
  // put your main code here, to run repeatedly:
  if(st.available()){
    if(cur == 0){
      digitalWrite(13,HIGH);
    }else{
      digitalWrite(13,LOW);
    }
    cur ^= 1;
    st.rxObj(act);
    //Serial.println(act);
  }
}