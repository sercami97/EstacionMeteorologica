#include <SoftwareSerial.h>

//SIM800 TX is connected to Arduino D8
#define SIM800_TX_PIN 5

//SIM800 RX is connected to Arduino D7
#define SIM800_RX_PIN 4

//Create software serial object to communicate with SIM800
SoftwareSerial serialSIM800(SIM800_TX_PIN,SIM800_RX_PIN);

#define transistor_sim 9   //Pin del transistor


void setup() {
//Begin serial comunication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(9600);
  while(!Serial);

  pinMode(transistor_sim, OUTPUT);
  delay(100);
  digitalWrite(transistor_sim, HIGH);
  delay(100);
  
  //Being serial communication witj Arduino and SIM800
  serialSIM800.begin(9600);
  delay(1000);
  
  Serial.println("Setup Complete!");
}

void loop() {
//Read SIM800 output (if available) and print it in Arduino IDE Serial Monitor
  if (serialSIM800.available())
  
  Serial.write(serialSIM800.read());
  
  if (Serial.available()){
  
    while(Serial.available()){
      serialSIM800.write(Serial.read());
  }
  serialSIM800.println();
  
  }
  

}
