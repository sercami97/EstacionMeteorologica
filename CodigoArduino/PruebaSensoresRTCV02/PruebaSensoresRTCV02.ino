#include "DHT.h" // Libreria DHT de Adafruit para DHT 11 o DHT 22 https://github.com/adafruit/DHT-sensor-library/archive/master.zip
#include <DS3232RTC.h>  //Libreria RTC https://github.com/JChristensen/DS3232RTC
#include "SIM800L.h"
#include <SoftwareSerial.h>



#define transistor_Pin 12   //Pin del transistor
#define transistor_sim 7   //Pin del transistor
#define transistor_aux 9   //Pin del transistor


#define rled 3   //Pin del transistor
#define gled 5   //Pin del transistor
#define bled 6   //Pin del transistor

//------------------HALL----------------
#define  hall_an 10  //Pin Anemometro (Vel. Viento)
#define  hall_pluv1 13 //Pin Anemometro (Pluviografo1)

#define DHTPIN 11     //Pin donde está conectado el sensor DHT
#define DHTTYPE DHT22   //Sensor DHT22
DHT dht(DHTPIN, DHTTYPE);

int hallCounter = 0;   //Contador tic tocs anemometro
int hallState = 0;         //Estado actual del sensor efecto hall anemometro (HIGH,LOW)

int lastHall = 0;     //Estado previo del sensor efecto hall anemometro (HIGH,LOW)
int lLastHall = 0;     //Estado previo del sensor efecto hall anemometro (HIGH,LOW)
int lLLastHall = 0;     //Estado previo del sensor efecto hall anemometro (HIGH,LOW)

int hallCounter1 = 0;   //Contador tic tocs ´pluviografo
int hallState1 = 0;      //Estado actual del sensor efecto hall pluviografo (HIGH,LOW)

bool lastHall1 = HIGH;    //Estado previo del sensor efecto hall pluviografo (HIGH,LOW)
bool lLastHall1 = HIGH;     //Estado previo del sensor efecto hall anemometro (HIGH,LOW)
bool lLLastHall1 = HIGH;     //Estado previo del sensor efecto hall anemometro (HIGH,LOW)

float starttime;
float new_endtime = 0;

int tiempo_hall = 100;

int direccion = 0;
int WV = 0;



float h;  //Variable de humedad relativa (0-100)
float temp; //Variable temeperatura grados centigrados

boolean date_done = false;

float vol_bat = 0;
float vol_panel = 0;

#define RST_ARD 4

//#define SIM800_RX_PIN 5
//#define SIM800_TX_PIN 4
#define SIM800_RST_PIN 8

const char APN[] = "hologram";
const char USR[] = "";
const String URL = "https://eco.agromakers.org/api/r_v?id=2020080427&tramo=";
const String URL2 = "https://eco.agromakers.org/api/r_v?id=2020080427&tramo=";

SIM800L* sim800l;


const uint8_t LIMIT_CONT = 35;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  //delay(2000);
  pinMode(RST_ARD, OUTPUT); 
  delay(500);
  digitalWrite(RST_ARD, LOW);
  
  pinMode(transistor_Pin, OUTPUT);
  pinMode(transistor_sim, OUTPUT);
  pinMode(transistor_aux, OUTPUT);

  testRGB();
  


  Serial.println("--- SIM ON ----");
  digitalWrite(transistor_sim, HIGH);
  delay(100);

  Serial.println("--- SENS ON ----");
  digitalWrite(transistor_Pin, HIGH);
  delay(100);
  
  Serial.println("--- AUX ON ----");
  digitalWrite(transistor_aux, HIGH);
  delay(100);
  
//  delay(10000);
//
//  Serial.println("--- SIM OFF ----");
//  digitalWrite(transistor_sim, LOW);
//  delay(100);
//
//  Serial.println("--- SENS OFF ----");
//  digitalWrite(transistor_sim, LOW);
//  delay(100);
//  
//  Serial.println("--- AUX OFF ----");
//  digitalWrite(transistor_aux, LOW);
//  delay(100);

 // Equivalent line with the debug enabled on the Serial
  sim800l = new SIM800L((Stream *)&Serial, SIM800_RST_PIN, 300, 20, (Stream *)&Serial);

  delay(5000);

//  sendGet(URL);
  
  dht.begin();//Inicializar el DHT
  delay(500);

  temp_hum();
  delay(500);

  initTime();
  delay(1000);
  time_t tiempo = RTC.get();
  Serial.println(F("Time Datos"));
  Serial.println((String(hour(tiempo))+ "," + String(minute(tiempo))));
  
}

void loop() {
  

  time_t t = RTC.get();
  Serial.println(F("HORA"));
  Serial.println(String(hour(t))+ "," + String(minute(t)) + "," + String(second(t)));

  delay(1000);

  
}
void testRGB(){
  pinMode(rled, OUTPUT);
  pinMode(bled, OUTPUT);
  pinMode(gled, OUTPUT);
  
  analogWrite(rled, 0);
  analogWrite(bled, 0);
  analogWrite(gled, 0);

  //Test de color
  Serial.println("1");
  analogWrite(rled, 255); // Se enciende color rojo
  delay(1000);            // Se esperan 1 s
  analogWrite(rled, 50); // Se enciende color rojo
  delay(1000);            // Se esperan 1 s
  analogWrite(rled, 0);  // Se apaga color rojo
  
  Serial.println("2");
  analogWrite(bled, 255); // Se enciende color azul
  delay(1000);            // Se esperan 1 s
  analogWrite(bled, 50); // Se enciende color azul
  delay(1000);            // Se esperan 1 s
  analogWrite(bled, 0);  // Se apaga color azul

  Serial.println("3");
  analogWrite(gled, 255); // Se enciende color verde
  delay(1000);            // Se esperan 1 s
  analogWrite(gled, 50); // Se enciende color verde
  delay(1000);            // Se esperan 1 s
  analogWrite(gled, 0);  // Se apaga colo verde
}

void testSensores(){
  updateVolt();
  delay(1000);
  int dirF = 0;
  temp_hum();
  delay(500);
  int contDifDir = 0;
  boolean out = true;
  int dir1 = dir();
  delay(1000);

//  while(true){
//    int dir1 = dir();
//    delay(1000);
//    int dir2 = dir();
//    delay(1000);
//    int dir3 = dir();
//    delay(1000);
//    int dir4 = dir();
//    delay(1000);
//    if(dir1 == dir2){
//      dirF = dir1;
//      out = false;;  
//    }else{
//      contDifDir++;
//      dirF = dir1;
//    }
//  }
    

  hall();
  delay(100000);
}
void initTime(){
  //Configuración de la fecha
  RTC.begin();
  delay(1000);
  
  if (true){
  tmElements_t tm;
  tm.Hour = 16;
  tm.Minute = 11;
  tm.Second = 00;
  tm.Day = 13;
  tm.Month = 07;
  tm.Year = 2021 - 1970;
  RTC.write(tm); 

  time_t t = RTC.get();
  Serial.println(String(hour(t))+ "," + String(minute(t)));
  }
  
  date_done = true;
}

void hall() {
  hallCounter = 0;
  hallCounter1 = 0;
  starttime = millis();
  Serial.print("Inicio Hall");
  //Serial.println(starttime);
  while ((new_endtime - starttime) < tiempo_hall) { //Realizar este loop durante 45 segundos
    hallState = digitalRead(hall_an); //Estado hall anemometro
    hallState1 = digitalRead(hall_pluv1); //Estado hall pluviografo 1
//    Serial.println("----------------------");
//    if (hallState == HIGH) {
//      Serial.println(F("HIGH"));
//    }else{
//      Serial.println(F("LOW"));
//    }
//    if (lastHall1 == HIGH) {
//      Serial.println(F("HIGH"));
//    }else{
//      Serial.println(F("LOW"));
//    }
//    if (lLastHall == HIGH) {
//      Serial.println(F("HIGH"));
//    }else{  
//      Serial.println(F("LOW"));
//    }
//    if (lLLastHall == HIGH) {
//      Serial.println(F("HIGH"));
//    }else{
//      Serial.println(F("LOW"));
//    }
    //Si hay un cambio en el estado HIGH ->LOW o LOW->HIGH en la señal de salida de los efecto hall
    //se aumenta en 1 el numero de tic tocs
    if (lLLastHall != lLastHall) { 
      if (hallState == HIGH and lastHall == HIGH and lLastHall == HIGH) {
        hallCounter++;
        Serial.println(hallCounter);
        Serial.println("Anemometro");
//delay(500);
      }
      delay(50);
    }
    if (lLLastHall1 != lLastHall1) {
      if (hallState1 == HIGH and lastHall1 == HIGH and lLastHall1 == HIGH) {
        hallCounter1++;
        Serial.println(hallCounter1);
        Serial.println("Pluviografo");
        //delay(500);
      }
      delay(50);
    }
    
    lLLastHall = lLastHall;
    lLastHall = lastHall;
    lastHall = hallState;

    lLLastHall1 = lLastHall1;
    lLastHall1 = lastHall1;
    lastHall1 = hallState1;

    
    new_endtime = millis();   
  }
  Serial.println("Counter"); 
  Serial.println(hallCounter);
  Serial.println(hallCounter1);  
}


int dir(){

  //Dependiendo del valor analogo obtenido de la señal del sensor infrarrojo se establece la direccion
    WV = analogRead(A3);
    Serial.println("DIR ->");
    Serial.println(WV);
//--------------ESTACION 5-------------
    if (WV > 145 && WV < 165){
      direccion = 1;
    }
    else if (WV >= 166 && WV < 180){
      direccion = 2;
    }
    else if (WV >= 181 && WV < 200){
      direccion = 3;
    }
    else if (WV >= 201 && WV < 220){
      direccion = 4;
    }
    else if (WV >= 221 && WV < 246){
      direccion = 5;
    }  
    else if (WV >= 246 && WV < 275){
      direccion = 6;
    }
    else if (WV >= 276 && WV < 305){
      direccion = 7;
    }
    else if (WV >= 305 && WV < 413){
      direccion = 8;
    }
    else{
      direccion = 9;
    }
//------------- ESTACION4 ---------------
//    if (WV > 145 && WV < 180){
//      direccion = 1;
//    }
//    else if (WV >= 181 && WV < 195){
//      direccion = 2;
//    }
//    else if (WV >= 196 && WV < 210){
//      direccion = 3;
//    }
//    else if (WV >= 211 && WV < 245){
//      direccion = 4;
//    }
//    else if (WV >= 245 && WV < 275){
//      direccion = 5;
//    }  
//    else if (WV >= 276 && WV < 305){
//      direccion = 6;
//    }
//    else if (WV >= 306 && WV < 335){
//      direccion = 7;
//    }
//    else if (WV >= 336 && WV < 413){
//      direccion = 8;
//    }
//    else{
//      direccion = 9;
//    }

//------------ ESTACION2 -----------
//    if (WV > 145 && WV < 160){
//      direccion = 1;
//    }
//    else if (WV >= 161 && WV < 180){
//      direccion = 2;
//    }
//    else if (WV >= 181 && WV < 200){
//      direccion = 3;
//    }
//    else if (WV >= 201 && WV < 215){
//      direccion = 4;
//    }
//    else if (WV >= 216 && WV < 235){
//      direccion = 5;
//    }  
//    else if (WV >= 236 && WV < 249){
//      direccion = 6;
//    }
//    else if (WV >= 250 && WV < 265){
//      direccion = 7;
//    }
//    else if (WV >= 265 && WV < 413){
//      direccion = 8;
//    }
//    else{
//      direccion = 9;
//    }

//-------------- ESTACION3 ------------
//if (WV > 145 && WV < 180){
//      direccion = 1;
//    }
//    else if (WV >= 181 && WV < 205){
//      direccion = 2;
//    }
//    else if (WV >= 206 && WV < 225){
//      direccion = 3;
//    }
//    else if (WV >= 226 && WV < 245){
//      direccion = 4;
//    }
//    else if (WV >= 246 && WV < 265){
//      direccion = 5;
//    }  
//    else if (WV >= 266 && WV < 290){
//      direccion = 6;
//    }
//    else if (WV >= 291 && WV < 315){
//      direccion = 7;
//    }
//    else if (WV >= 316 && WV < 413){
//      direccion = 8;
//    }
//    else{
//      direccion = 9;
//    }

// ------------------- ESTACION1 ------------
//    if (WV > 130 && WV < 155){
//      direccion = 1;
//    }
//    else if (WV >= 156 && WV < 175){
//      direccion = 2;
//    }
//    else if (WV >= 176 && WV < 190){
//      direccion = 3;
//    }
//    else if (WV >= 191 && WV < 220){
//      direccion = 4;
//    }
//    else if (WV >= 221 && WV < 240){
//      direccion = 5;
//    }  
//    else if (WV >= 241 && WV < 255){
//      direccion = 6;
//    }
//    else if (WV >= 256 && WV < 279){
//      direccion = 7;
//    }
//    else if (WV >= 280 && WV < 350 ){
//      direccion = 8;
//    }
//    else{
//      direccion = 9;
//    }
    Serial.println(direccion);
    return direccion;
}

void temp_hum() {
  h = dht.readHumidity(); //Leer la humedad relativa
  temp = dht.readTemperature(); //Leer temperatura en grados Celsius
  Serial.println("Datos Ambiente");
  Serial.println(String(h) + " - " + String(temp));
}

void updateVolt(){
  vol_bat = float (analogRead(A1)*(0.0049)*2);
  vol_panel = float (analogRead(A2)*(4.73)*(0.0049));
  Serial.println(String(vol_bat) + " - " + String(vol_panel));  
}

//bool sendGet(String msg_param) {
//    // Setup module for GPRS communication
//  bool enviado = false; 
//  uint8_t contfail = 0;
//  uint8_t contFailSetup = 0;
//    
//  while(!enviado and contfail<5){
//    boolean var1;
//    contFailSetup = 0;
//    while(contFailSetup <= 2){
//      var1 = setupModule();
//      if(!var1){
//        contFailSetup++;
//        Serial.println(F("Failed setup cycle"));
//        Serial.println(var1);
//        Serial.println(F("Reset SIM"));
////        sim800l->reset();
////        delay(5000); 
//      }else{
//        break;
//      }
//    }
////    if(var1 == false){
////      Serial.println(F("Var False"));
////    }else{
////      Serial.println(F("Var TRUE"));
////    }
//    if(contFailSetup>=4 and !var1){
//      Serial.println(F("Unable to Setup Module SIM"));
//      //return false;
//    }
////    // Close GPRS connectivity (5 trials)
////    bool disconnected2 = sim800l->disconnectGPRS();
////    for(uint8_t i = 0; i < 2 && !disconnected2; i++) {
////      delay(1000);
////      disconnected2 = sim800l->disconnectGPRS();
////    }
////    
////    if(disconnected2) {
////      Serial.println(F("GPRS disconnected !"));
////    } else {
////      Serial.println(F("GPRS still connected !"));
////    }
//    
//    // Establish GPRS connectivity (5 trials)
//    bool connected = false;
//    for(uint8_t i = 0; i < 2 && !connected; i++) {
//      delay(2000);
//      connected = sim800l->connectGPRS1();
//    }
//  
//    // Check if connected, if not reset the module and setup the config again
//    if(!connected) {
//      Serial.println(F("GPRS1 not connected !"));
//    }
//      
//    connected = false;
//    for(uint8_t i = 0; i < 5 && !connected; i++) {
//      delay(1000);
//      connected = sim800l->connectGPRS2();
//    }
//  
//    // Check if connected, if not reset the module and setup the config again
//    if(connected) {
//      Serial.println(F("GPRS2 connected !"));
//    } else {
//      Serial.println(F("GPRS2 not connected !"));
//    }
//    delay(5000);
//    Serial.println(F("Start HTTP GET..."));
//  
//    connected = false;
//    for(uint8_t i = 0; i < 5 && !connected; i++) {
//      delay(1000);
//      connected = sim800l->initiateHTTPINIT();
//    }
//  
//    // Check if connected, if not reset the module and setup the config again
//    if(!connected) {
//      Serial.println(F("HTTP FAIL !"));
//    }
//    Serial.println(F("URL HTTP"));
//    Serial.println(msg_param);
//    connected = false;
//    for(uint8_t i = 0; i < 5 && !connected; i++) {
//      delay(5000);
//      connected = sim800l->initiateHTTPPARAINIT((msg_param).c_str(),NULL);
//    }
//  
//    // Check if connected, if not reset the module and setup the config again
//    if(connected) {
//      Serial.println(F("HTTPPARA SUCCESS !"));
//    } else {
//      Serial.println(F("HTTPPARA FAIL !"));
//    }
//
//    uint16_t contHttpError = 0;
//    while(enviado==false and contHttpError < 3){
//      // Do HTTP GET communication with 10s for the timeout (read)
//      uint16_t rc = sim800l->doGet(65000);
//       if(rc == 200 or rc == 409) {
//        // Success, output the data received on the serial
//        Serial.println(F("HTTP GET successful"));
//        enviado = true;
//        break;
//      } else {
//        // Failed...
//        Serial.print(F("HTTP GET error "));
//        Serial.println(rc);
//        contHttpError++;
//        delay(3000);
////        Serial.println("ContFail");
////        Serial.println(contfail);
//      }
//    }
//    delay(1000);
//    // Close GPRS connectivity (5 trials)
//    bool disconnected = sim800l->disconnectGPRS();
//    for(uint8_t i = 0; i < 2 && !connected; i++) {
//      delay(1000);
//      disconnected = sim800l->disconnectGPRS();
//    }
//    
//    if(!disconnected) {
//      Serial.println(F("GPRS still connected !"));
//    }
//    delay(5000);
//    // Go into low power mode
//    bool lowPowerMode = sim800l->setPowerMode(MINIMUM);
//    if(!lowPowerMode) {
//      Serial.println(F("Failed to switch module to low power mode"));
//      delay(500);
//      lowPowerMode = sim800l->setPowerMode(MINIMUM);
//      if(!lowPowerMode) {
//        Serial.println(F("Failed to switch module to low power mode x2"));
//      }
//    }
//    
//    delay(5000);
//    // Go into sleep mode
//    lowPowerMode = sim800l->sleepModeCLK();
//    if(!lowPowerMode) {
//      Serial.println(F("Failed to switch module to sleep mode"));
//    }
//    contfail++;
////    Serial.println(F("ContFail"));
////    Serial.println(contfail);
//
//  }
//  return enviado;
//}

//bool setupModule() {
//  bool resp = true;
//  uint8_t maxContSig = 0;
//    // Wait until the module is ready to accept AT commands
//  while(!sim800l->isReady() and maxContSig < LIMIT_CONT) {
//    //Serial.println(F("Problem to initialize AT command, retry in 1 sec"));
//    delay(500);
//    maxContSig++;
//  }
//  if(maxContSig>=LIMIT_CONT){
//    Serial.println(F("No AT Response"));
//    return false;
//  }
//  delay(500);
//  if(sim800l->sleepWakeCLK()) {
//    //Serial.println(F("Module awake mode"));
//  } else {
//    Serial.println(F("Failed to wake up module"));
//  }
//  
//  Serial.println(F("Setup Complete!"));
//  bool nornmalMode = sim800l->setPowerMode(NORMAL);
////  if(nornmalMode) {
////    Serial.println(F("Module in normal power mode"));
////  } else {
////    Serial.println(F("Failed to switch module to normal power mode"));
////  }
//  // Wait for the GSM signal
//  maxContSig = 0;
//  uint8_t signal = sim800l->getSignal();
//  while(signal <= 0 && maxContSig < LIMIT_CONT) {
//    delay(500);
//    signal = sim800l->getSignal();
//    Serial.print(signal);
//    maxContSig++;
//  }
//  if(maxContSig>=LIMIT_CONT){
//    Serial.println(F("No Signal"));
//    resp = false;
//  }
////  Serial.print(F("Signal OK (strenght: "));
////  Serial.print(signal);
////  Serial.println(F(")"));
////  delay(1000);
//
//  // Wait for operator network registration (national or roaming network)
//  maxContSig = 0;
//  NetworkRegistration network = sim800l->getRegistrationStatus();
//  while(network != REGISTERED_HOME && network != REGISTERED_ROAMING && maxContSig < LIMIT_CONT) {
//    delay(1000);
//    network = sim800l->getRegistrationStatus();
//    maxContSig++;
//  }
//  if(maxContSig>=LIMIT_CONT){
//    Serial.println(F("Network Registration Failed"));
//    //return false;
//  }
////  Serial.println(F("Network registration OK"));
////  delay(1000);
//
//  // Setup APN for GPRS configuration
//  bool success = sim800l->setupGPRS(APN);
//  while(!success) {
//    success = sim800l->setupGPRS(APN);
//    delay(5000);
//  }
//  //Serial.println(F("GPRS config OK"));
//
//  return resp;
//}
