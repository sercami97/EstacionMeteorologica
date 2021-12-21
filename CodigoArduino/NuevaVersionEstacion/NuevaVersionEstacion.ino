/********************************************************************************
 * Example of HTTPS GET with Serial1 (Mega2560) and Arduino-SIM800L-driver      *
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
#include "SIM800L.h"
#include <avr/sleep.h> //Libreria avr que contiene los metodos que controlan el modo sleep
#include <DS3232RTC.h>  //Libreria RTC https://github.com/JChristensen/DS3232RTC

//-------------- SIM800L ---------------------

const char APN[] = "hologram";
const char URL2[] = "http://eco.agromakers.org/api/date";
const char URL[] = "http://eco.agromakers.org/api/r_v?id=2020080427&tramo=";

SIM800L* sim800l;

//---------- MOSFETs ---------------
#define transistor_sen 12   //Pin del transistor
#define transistor_sim 7   //Pin del transistor
#define transistor_aux 9   //Pin del transistor

//------------- SYSTEM -----------
#define RST_ARD 4

uint16_t vol_bat;
uint8_t vol_panel; 

String finalString;
String msg;

const uint16_t MAX_BYTES = 200;

int len;

//--------------- RTC --------------
String time_str = "";

void setup() {  
  // Initialize Serial Monitor for debugging
  Serial.begin(9600);
  while(!Serial);
  
  Serial.println(F("--- BEGIN SETUP ----")); 
  
  pinMode(RST_ARD, OUTPUT);
  delay(100); 
  digitalWrite(RST_ARD, LOW);
  delay(200);
  
  initPins(); //Inicializar pines sensores efecto hall, dht, interrupt, transistor
  inicializarSM(); //Inicializar Sleep Mode
  initSim();
  


  initTime();
  delay(500);

  tomaDatos();
  delay(500);
  tomaDatos();
  delay(500);
  tomaDatos();
  delay(500);
  tomaDatos();
  delay(500);
  eliminarUltimoDigitoURL();
  
}


void loop() {

  if(sendGet(msg,false)==true){
    Serial.println("SUCCESS GET");
    remove_msg();
  }else{
    Serial.println("FAILED GET");    
  }
  while(1);
}


void initPins() {
  //Declarar pines de la señal de los sensores efecto hall como inputs
  //pinMode(interruptPin, INPUT_PULLUP);
  pinMode(transistor_sen, OUTPUT);
  pinMode(transistor_sim, OUTPUT);

  //pinMode(DTRpin, OUTPUT);
  //digitalWrite(DTRpin, LOW);
  
//  pinMode(hall_an, INPUT); 
//  pinMode(hall_pluv1, INPUT);
  
//  dht.begin();//Inicializar el DHT
}

void inicializarSM() {
  //Eliminar cualquier alarma previa
  RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
  RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.alarmInterrupt(ALARM_2, false);
  RTC.squareWave(SQWAVE_NONE);
}
void updateVolt(){
  vol_bat = uint16_t (analogRead(A1)*(0.0049)*2*100);
  vol_panel = uint8_t (analogRead(A2)*(4.73)*(0.0049)*10);
  //Serial.println(String(vol_bat) + " - " + String(vol_panel));  
}
void initSim(){
  digitalWrite(transistor_sim, HIGH);
  delay(1000); 
  // Initialize SIM800L driver with an internal buffer of 200 bytes and a reception buffer of 512 bytes, debug disabled
  sim800l = new SIM800L((Stream *)&Serial, NULL, MAX_BYTES, 20);
  digitalWrite(transistor_sim, LOW);
  delay(1000);
}




void initTime(){
  
  sendGet(URL2,true);

  time_str.remove(0,1);
//  Serial.println(time_str);

  delay(500);
  char *strings[10];
  char *ptr = NULL;
  int tam = time_str.length();
  char value[tam];
  time_str.toCharArray(value, tam);
  byte index = 0;
  
  ptr = strtok(value, ",");  // takes a list of delimiters
  while(ptr != NULL)
  {
      strings[index] = ptr;
      index++;
      ptr = strtok(NULL, ",");  // takes a list of delimiters
  }

   String hr,mn,dd,mm;
   
   hr = strings[2];
   mn = strings[3];
   dd = strings[0];
   mm = strings[1];
 
  RTC.begin();
  delay(500);

  int dayy = dd.toInt();
  int monthh = mm.toInt();

  tmElements_t tm;
  tm.Hour = hr.toInt();
  tm.Minute = mn.toInt();
  tm.Second = 00;
  tm.Day = dayy;
  tm.Month = monthh;
  tm.Year = 2021 - 1970;
  RTC.write(tm); 
  delay(1000);
  
  Serial.println(F("END Time Init"));
}
void tomaDatos() {
  digitalWrite(transistor_sen, HIGH); //Habilitar nodo 5V de alimentacion sensores
  delay(100);
  
  time_t tiempo = RTC.get();

  //Guardar datos en un String
  updateVolt();

  finalString.reserve(40); //Se vuelve a disponer 40bytes de memoria para el nuevo mensaje
  finalString = String(day(tiempo))+ "," + String(month(tiempo))+ "," + String(hour(tiempo))+ "," + String(minute(tiempo)) + "," + String(10) + "," + String(50) + "," + String(1) + "," + String(9) + "," + String(2) + "," + String(vol_panel) + "," + String(vol_bat)+";";
   
  msg = msg + finalString; //Guardar la toma de diferentes ciclos en una variable que sera enviada al servidor
  finalString.remove(0,40); //Se elimina el mensaje enviado
  //Serial.println(msg);
  
  digitalWrite(transistor_sen, LOW); //Desabilitar nodo 5V de alimentacion sensores
  delay(100);
}
void remove_msg(){
  //Serial.println(F("REMOVE MSG"));
//  ciclo = 0;
  delay(200);
  msg.remove(0,MAX_BYTES); //Se elimina el mensaje enviado
  msg.reserve(MAX_BYTES); //Se vuelve a disponer 290bytes de memoria para el nuevo mensaje
  delay(1000);    
}

void eliminarUltimoDigitoURL(){
  //Serial.println(msg);
  Serial.println(F("BORRAR ULTIMO ADD URL"));
  msg = URL + msg;
  
  len = msg.length();
  msg.remove(len-1,1);
  //Serial.println(msg);
  delay(2000);
  Serial.println(F("FIN BORRAR ULTIMO ADD URL"));

}
bool sendGet(String URL_param, boolean init_time){

//  Serial.println("--- SIM ON ----");
  digitalWrite(transistor_sim, HIGH); //Habilitar nodo 5V de alimentacion sensores
  delay(5000);

  setupModule();
  delay(1500);
  
  // Establish GPRS connectivity (5 trials)
  bool sent = false;
  bool connected = false;
  for(uint8_t i = 0; i < 5 && !connected; i++) {
    delay(1000);
    connected = sim800l->connectGPRS();
  }

  // Check if connected, if not reset the module and setup the config again
  if(connected) {
    Serial.print(F("GPRS connected with IP "));
    Serial.println(sim800l->getIP());
  } else {
    Serial.println(F("GPRS not connected !"));
    Serial.println(F("Reset the module."));
    sim800l->reset();
    setupModule();
    return;
  }

  Serial.println(F("Start HTTP GET..."));

  // Do HTTP GET communication with 10s for the timeout (read)
  uint16_t rc = sim800l->doGet(URL_param.c_str(), 10000);
   if(rc == 200) {
    // Success, output the data received on the serial
    Serial.print(F("HTTP GET successful ("));
    Serial.print(sim800l->getDataSizeReceived());
    Serial.println(F(" bytes)"));
    Serial.print(F("Received : "));
    Serial.println(sim800l->getDataReceived());
    Serial.print(F("HTTP CODE : "));
    Serial.println(rc);
    sent = true;
    if(init_time){time_str = sim800l->getDataReceived();}

  } else {
    // Failed...
    Serial.print(F("HTTP GET error "));
    Serial.println(rc);
  }

  // Close GPRS connectivity (5 trials)
  bool disconnected = sim800l->disconnectGPRS();
  for(uint8_t i = 0; i < 5 && !connected; i++) {
    delay(1000);
    disconnected = sim800l->disconnectGPRS();
  }
  
  if(disconnected) {
    Serial.println(F("GPRS disconnected !"));
  } else {
    Serial.println(F("GPRS still connected !"));
  }

  // Go into low power mode
  bool lowPowerMode = sim800l->setPowerMode(MINIMUM);
  if(lowPowerMode) {
    Serial.println(F("Module in low power mode"));
  } else {
    Serial.println(F("Failed to switch module to low power mode"));
  }

//  Serial.println("--- SIM OFF ----");
  digitalWrite(transistor_sim, LOW); //Habilitar nodo 5V de alimentacion sensores
  delay(5000);

  // End of program... wait...
  return sent;
}

void setupModule() {
    // Wait until the module is ready to accept AT commands
  while(!sim800l->isReady()) {
    Serial.println(F("Problem to initialize AT command, retry in 1 sec"));
    delay(1000);
  }
  Serial.println(F("Setup Complete!"));

  // Wait for the GSM signal
  uint8_t signal = sim800l->getSignal();
  while(signal <= 0) {
    delay(1000);
    signal = sim800l->getSignal();
  }
  Serial.print(F("Signal OK (strenght: "));
  Serial.print(signal);
  Serial.println(F(")"));
  delay(1000);

  // Wait for operator network registration (national or roaming network)
  NetworkRegistration network = sim800l->getRegistrationStatus();
  while(network != REGISTERED_HOME && network != REGISTERED_ROAMING) {
    delay(1000);
    network = sim800l->getRegistrationStatus();
  }
  Serial.println(F("Network registration OK"));
  delay(1000);

  // Setup APN for GPRS configuration
  bool success = sim800l->setupGPRS(APN);
  while(!success) {
    success = sim800l->setupGPRS(APN);
    delay(5000);
  }
  Serial.println(F("GPRS config OK"));
}
