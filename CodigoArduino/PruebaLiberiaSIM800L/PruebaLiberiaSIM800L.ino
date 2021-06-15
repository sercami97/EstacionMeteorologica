/********************************************************************************
 * Example of HTTPS GET with SoftwareSerial and Arduino-SIM800L-driver          *
 *                                                                              *
 * Author: Olivier Staquet                                                      *
 * Last version available on https://github.com/ostaquet/Arduino-SIM800L-driver *
 ********************************************************************************
 * MIT License
 *
 * Copyright (c) 2019 Olivier Staquet
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *******************************************************************************/
#include <SoftwareSerial.h>

#include "SIM800L.h"

#define SIM800_RX_PIN 5
#define SIM800_TX_PIN 4
#define SIM800_RST_PIN 10

const char APN[] = "internet.comcel.com.co";
const char URL[] = "https://eco.agromakers.org/api/r_v?id=2021041229&tramo=1,1,1,1,1,1,1,1,1,1,1;2,2,2,2,2,2,2,2,2,2,2";

SIM800L* sim800l;

const uint8_t LIMIT_CONT = 35;


void setup() {  
  // Initialize Serial Monitor for debugging
  Serial.begin(9600);
  while(!Serial);

  // Initialize a SoftwareSerial
  SoftwareSerial* serial = new SoftwareSerial(SIM800_RX_PIN, SIM800_TX_PIN);
  serial->begin(9600);
  delay(1000);
   
  // Initialize SIM800L driver with an internal buffer of 200 bytes and a reception buffer of 512 bytes, debug disabled
  //sim800l = new SIM800L((Stream *)serial, SIM800_RST_PIN, 256, 256);

  // Equivalent line with the debug enabled on the Serial
  sim800l = new SIM800L((Stream *)serial, SIM800_RST_PIN, 256, 256, (Stream *)&Serial);

  delay(10000);

}
 
void loop(){
  if(!sendGet()){
    Serial.println(F("ERROR ENVIO"));
  }else{
    Serial.println(F("SUCCESS ENVIO"));
  }
  delay(5000);
}

bool sendGet() {
    // Setup module for GPRS communication
  bool enviado = false; 
  uint8_t contfail = 0;
  uint8_t contFailSetup = 0;
    
  while(!enviado and contfail<5){
    boolean var1;
    contFailSetup = 0;
    while(contFailSetup <= 2){
      var1 = setupModule();
      if(!var1){
        contFailSetup++;
        Serial.println(F("Failed setup cycle"));
        Serial.println(var1);
        Serial.println(F("Reset SIM"));
//        sim800l->reset();
//        delay(5000); 
      }else{
        break;
      }
    }
//    if(var1 == false){
//      Serial.println(F("Var False"));
//    }else{
//      Serial.println(F("Var TRUE"));
//    }
    if(contFailSetup>=4 and !var1){
      Serial.println(F("Unable to Setup Module SIM"));
      //return false;
    }
    
    // Establish GPRS connectivity (5 trials)
    bool connected = false;
    for(uint8_t i = 0; i < 2 && !connected; i++) {
      delay(2000);
      connected = sim800l->connectGPRS1();
    }
  
    // Check if connected, if not reset the module and setup the config again
    if(!connected) {
      Serial.println(F("GPRS1 not connected !"));
    }
      
    connected = false;
    for(uint8_t i = 0; i < 5 && !connected; i++) {
      delay(1000);
      connected = sim800l->connectGPRS2();
    }
  
    // Check if connected, if not reset the module and setup the config again
    if(connected) {
      Serial.println(F("GPRS2 connected !"));
    } else {
      Serial.println(F("GPRS2 not connected !"));
    }
    delay(5000);
    Serial.println(F("Start HTTP GET..."));
  
    connected = false;
    for(uint8_t i = 0; i < 5 && !connected; i++) {
      delay(1000);
      connected = sim800l->initiateHTTPINIT();
    }
  
    // Check if connected, if not reset the module and setup the config again
    if(!connected) {
      Serial.println(F("HTTP FAIL !"));
    }
    Serial.println(F("URL HTTP"));
    connected = false;
    for(uint8_t i = 0; i < 5 && !connected; i++) {
      delay(5000);
      connected = sim800l->initiateHTTPPARAINIT(URL,NULL);
    }
  
    // Check if connected, if not reset the module and setup the config again
    if(connected) {
      Serial.println(F("HTTPPARA SUCCESS !"));
    } else {
      Serial.println(F("HTTPPARA FAIL !"));
    }

    uint16_t contHttpError = 0;
    while(enviado==false and contHttpError < 3){
      // Do HTTP GET communication with 10s for the timeout (read)
      uint16_t rc = sim800l->doGet(65000);
       if(rc == 200) {
        // Success, output the data received on the serial
        //Serial.println(F("HTTP GET successful"));
        enviado = true;
      } else {
        // Failed...
        Serial.print(F("HTTP GET error "));
        Serial.println(rc);
        contHttpError++;
        delay(3000);
//        Serial.println("ContFail");
//        Serial.println(contfail);
      }
    }
    delay(1000);
    // Close GPRS connectivity (5 trials)
    bool disconnected = sim800l->disconnectGPRS();
    for(uint8_t i = 0; i < 2 && !connected; i++) {
      delay(1000);
      disconnected = sim800l->disconnectGPRS();
    }
    
    if(!disconnected) {
      Serial.println(F("GPRS still connected !"));
    }
    delay(5000);
    // Go into low power mode
    bool lowPowerMode = sim800l->setPowerMode(MINIMUM);
    if(!lowPowerMode) {
      Serial.println(F("Failed to switch module to low power mode"));
      delay(500);
      lowPowerMode = sim800l->setPowerMode(MINIMUM);
      if(!lowPowerMode) {
        Serial.println(F("Failed to switch module to low power mode x2"));
      }
    }
    
    delay(5000);
    // Go into sleep mode
    lowPowerMode = sim800l->sleepModeCLK();
    if(!lowPowerMode) {
      Serial.println(F("Failed to switch module to sleep mode"));
    }
    contfail++;
//    Serial.println(F("ContFail"));
//    Serial.println(contfail);

  }
  return enviado;
}

bool setupModule() {
  bool resp = true;
  uint8_t maxContSig = 0;
    // Wait until the module is ready to accept AT commands
  while(!sim800l->isReady() and maxContSig < LIMIT_CONT) {
    //Serial.println(F("Problem to initialize AT command, retry in 1 sec"));
    delay(500);
    maxContSig++;
  }
  if(maxContSig>=LIMIT_CONT){
    Serial.println(F("No AT Response"));
    return false;
  }
  delay(500);
  if(sim800l->sleepWakeCLK()) {
    //Serial.println(F("Module awake mode"));
  } else {
    Serial.println(F("Failed to wake up module"));
  }
  
  Serial.println(F("Setup Complete!"));
  bool nornmalMode = sim800l->setPowerMode(NORMAL);
//  if(nornmalMode) {
//    Serial.println(F("Module in normal power mode"));
//  } else {
//    Serial.println(F("Failed to switch module to normal power mode"));
//  }
  // Wait for the GSM signal
  maxContSig = 0;
  uint8_t signal = sim800l->getSignal();
  while(signal <= 0 && maxContSig < LIMIT_CONT) {
    delay(500);
    signal = sim800l->getSignal();
    Serial.print(signal);
    maxContSig++;
  }
  if(maxContSig>=LIMIT_CONT){
    Serial.println(F("No Signal"));
    resp = false;
  }
//  Serial.print(F("Signal OK (strenght: "));
//  Serial.print(signal);
//  Serial.println(F(")"));
//  delay(1000);

  // Wait for operator network registration (national or roaming network)
  maxContSig = 0;
  NetworkRegistration network = sim800l->getRegistrationStatus();
  while(network != REGISTERED_HOME && network != REGISTERED_ROAMING && maxContSig < LIMIT_CONT) {
    delay(1000);
    network = sim800l->getRegistrationStatus();
    maxContSig++;
  }
  if(maxContSig>=LIMIT_CONT){
    Serial.println(F("Network Registration Failed"));
    //return false;
  }
//  Serial.println(F("Network registration OK"));
//  delay(1000);

  // Setup APN for GPRS configuration
  bool success = sim800l->setupGPRS(APN);
  while(!success) {
    success = sim800l->setupGPRS(APN);
    delay(5000);
  }
  //Serial.println(F("GPRS config OK"));

  return resp;
}
