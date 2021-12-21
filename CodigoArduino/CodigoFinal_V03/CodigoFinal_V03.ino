#include "Arduino.h"
#include "DHT.h" // Libreria DHT de Adafruit para DHT 11 o DHT 22 https://github.com/adafruit/DHT-sensor-library/archive/master.zip
#include "SIM800L.h"
#include <ESP32Time.h>
#include "PCF8574.h"

#include <driver/adc.h>

#include <EEPROM.h>
#include <WiFi.h>
#include <BluetoothSerial.h>
#include <esp_bt.h>


// Set i2c address
PCF8574 pcf8574(0x20);

#define hall1_vel 25   //Pin del transistor
#define hall2_vel 26   //Pin del transistor

// ---------------------- PLUVIOGRAFO -------------
#define HALL_PLUVI 27

uint8_t hallCounter = 0;   //Contador tic tocs anemometro
uint8_t hallState = 0;         //Estado actual del sensor efecto hall anemometro (HIGH,LOW)
uint8_t lastHall = 0;     //Estado previo del sensor efecto hall anemometro (HIGH,LOW)
uint8_t lLastHall = 0;     //Estado previo del sensor efecto hall anemometro (HIGH,LOW)
uint8_t lLLastHall = 0;     //Estado previo del sensor efecto hall anemometro (HIGH,LOW)

uint8_t hallCounter1 = 0;   //Contador tic tocs ´pluviografo
uint8_t hallState1 = 0;      //Estado actual del sensor efecto hall pluviografo (HIGH,LOW)
uint8_t lastHall1 = 0;    //Estado previo del sensor efecto hall pluviografo (HIGH,LOW)
uint8_t lLastHall1 = 0;     //Estado previo del sensor efecto hall anemometro (HIGH,LOW)
uint8_t lLLastHall1 = 0;     //Estado previo del sensor efecto hall anemometro (HIGH,LOW)

unsigned long tiempo_hall = 45000;

//---------------------- RTC ----------------------
ESP32Time rtc;

String time_str = "";
//---------------------- END RTC ----------------------

//const char APN[] = "hologram";
const char APN[] = "pepper";
const char USR[] = "comcel";
const char URL[] = "http://eco.agromakers.org/api/r_v?id=2020080427&tramo=";
const char URL2[] = "http://eco.agromakers.org/api/date";

SIM800L* sim800l;

//------------------ END HALL --------------

//---------- MOSFETs ---------------
#define transistor_sen 15   //Pin del transistor
#define transistor_sim 33   //Pin del transistor
#define transistor_aux 32   //Pin del transistor

//---------------- SLEEP MODE & RTC ---------------
//Variables 
//Variables de tiempo para el sensado del anemometro y el pluviografo
unsigned long starttime;
unsigned long new_endtime = 0;
int len;

//Numero de tomas y Sleep Mode
#define interruptPin 2 //Pin de interrupcion para despertar el arduino

const uint8_t time_interval = 1; //Intervalo de tiempo para la toma de datos
const uint8_t num_ciclos = 8; //Definir numero de tomas previas al envío (tomar 6 como valor máximo para evitar problemas de inestabilidad)

RTC_DATA_ATTR uint8_t ciclo = 0; 

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  900        /* Time ESP32 will go to sleep (in seconds) */

#define maxRecords 8 // Nombre max enregistrements

RTC_DATA_ATTR int bootCount = 0;

//----------------END SLEEP MODE & RTC ---------------


// ---------------- ESTADOS -------------
//Estado
uint8_t state = 1;
uint8_t last_state = 1;
  
const uint8_t DORMIR PROGMEM = 2;
const uint8_t DATOS PROGMEM = 1;
const uint8_t ENVIO PROGMEM = 3;
const uint8_t RESET_SIM PROGMEM = 5;
const uint8_t RESET_ARDU PROGMEM = 6;
const uint8_t ASK_BAT PROGMEM = 7;
const uint8_t INIT PROGMEM = 0;

const uint16_t MIN_VOLT_BAT = 345;
const uint8_t MIN_VOLT_PAN = 900;

boolean wait = false;

const uint8_t LIMIT_CONT = 35;


// ------------------ END ESTADOS -------------
// --------------SYSTEM VARIABLES -------------
//Outputs
//Strings en donde se almacenan los datos tomados
String finalString;

String msg;

boolean first_init = true;
boolean first_run = true;

int vol_bat;
int vol_panel; 

uint8_t cont_rst_sim = 0;

#define RST_ARD 4

const uint16_t MAX_BYTES = 500;

typedef struct {
  uint8_t Dia;
  uint8_t Mes;
  uint8_t Hora;
  uint8_t Minuto;

  uint8_t Dir;
  uint8_t Vel;
  uint8_t Pluv;
  uint16_t Temp;
  uint16_t Hum;

  uint16_t Panel;
  uint16_t Bateria;
} bmp180Records;


RTC_DATA_ATTR bmp180Records Records[maxRecords];

// --------------END SYSTEM VARIABLES -------------


#define PIN_BAT 36
#define PIN_PANEL 39



// ----------- RGB ---------------

// Set up the rgb led names
uint8_t ledR = 19;
uint8_t ledG = 18;
uint8_t ledB = 5; 

uint8_t ledArray[3] = {1, 2, 3}; // three led channels

const boolean invert = true; // set true if common anode, false if common cathode

uint8_t color = 0;          // a value from 0 to 255 representing the hue
uint32_t R, G, B;           // the Red Green and Blue color components
uint8_t brightness = 255;  // 255 is maximum brightness, but can be changed.  Might need 256 for common anode to fully turn off.

// --------------------- DHT ---------------------
#define DHTPIN 13     //Pin donde está conectado el sensor DHT
#define DHTTYPE DHT22   //Sensor DHT22
DHT dht(DHTPIN, DHTTYPE);
uint8_t h;  //Variable de humedad relativa (0-100)
uint16_t temp; //Variable temeperatura grados centigrados



//Que se cuenten envios de rrores pero que siga los siguientes estados. Reset cuando hallan 3 mensajes que no se han enviado.
void setup() {
  // Initialize Serial Monitor for debugging
  Serial.begin(9600);
  Serial2.begin(57600);
  while(!Serial);
  delay(500);

  initRGB();

  led_verde(5,1000);
  
  setModemSleep();

  Serial.print(F("Ciclo Numero "));
  Serial.println(ciclo);

  if(revisar_bat()){
    init_setup();
    if(bootCount==0){
      Serial.println(F("FIRST RUN"));
      led_verde(10,500);
      update_time();
      init_RTC_memory();
      state = INIT;  
    }else{
      state = DATOS;
    }
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));
  
    //Print the wakeup reason for ESP32
    print_wakeup_reason();
  
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds"); 
  }else{
    state = DORMIR;
  }
  
  while(true){
    loop_edited();
  }
}
void init_sensors(){

  digitalWrite(transistor_aux, HIGH); //Habilitar nodo 5V de alimentacion sensores
  digitalWrite(transistor_sen, HIGH); //Habilitar nodo 5V de alimentacion sensores

  delay(5000);

  pinMode(hall1_vel, INPUT_PULLUP);
  pinMode(hall2_vel, INPUT_PULLUP);
  
  pcf8574.pinMode(P0, INPUT_PULLUP);
  pcf8574.pinMode(P1, INPUT_PULLUP);
  pcf8574.pinMode(P2, INPUT_PULLUP);
  pcf8574.pinMode(P3, INPUT_PULLUP);
  pcf8574.pinMode(P4, INPUT_PULLUP);
  pcf8574.pinMode(P5, INPUT_PULLUP);
  pcf8574.pinMode(P6, INPUT_PULLUP);
  pcf8574.pinMode(P7, INPUT_PULLUP);

  Serial.print("Init pcf8574...");
  if (pcf8574.begin()){
    Serial.println("OK");
  }else{
    Serial.println("KO");
  }

 dht.begin();//Inicializar el DHT

 pinMode(HALL_PLUVI, INPUT); 

 delay(5000);
 
 digitalWrite(transistor_aux, LOW); //Habilitar nodo 5V de alimentacion sensores
 digitalWrite(transistor_sen, LOW); //Habilitar nodo 5V de alimentacion sensores
}
void init_sim800(){
  digitalWrite(transistor_sim, HIGH); //Habilitar nodo 5V de alimentacion sensores
  delay(500);

  sim800l = new SIM800L((Stream *)&Serial2, NULL, MAX_BYTES, 20, (Stream *)&Serial);
//  sim800l = new SIM800L((Stream *)&Serial2, NULL, MAX_BYTES, 20);

  digitalWrite(transistor_sim, LOW); //Habilitar nodo 5V de alimentacion sensores
  delay(500);  
}
void init_setup(){
  Serial.println(F("--- BEGIN SETUP ----")); 
    
  inicializarPines(); //Inicializar pines sensores efecto hall, dht, interrupt, transistor
  init_sensors();


  analogSetAttenuation(ADC_11db);

  msg.reserve(MAX_BYTES); //Reservar memoria en bytes para la cadena de caracteres de los datos
  delay(1000);

  //update_time();
  //updateVolt();  
}
void init_state(){
  led_rojo_amarillo(5,1000);
  
  ciclo = 0;
  for(int i = 0; i < 2; i++){
    tomaDatos();
    delay(1000);
    ciclo++;
  }
  
  ciclo = 0;
  
  merge_data_rtc(2);
  eliminarUltimoDigitoURL();
  
  Serial.println(F("Begin Send"));
  if(sendGet(msg,false)){
    state = DATOS;
    remove_msg();
    Serial.println(F("Success Send"));

  }else{
    state = DATOS;
    Serial.println(F("Fail Send"));
    led_rojo(10,500);
  }
}

void loop(){
  Serial.println("LOOP");
  updateVolt();
  delay(1000);
}
void go_sleep() {
  Serial.println("Going to sleep now");
  delay(1000);
  Serial.flush();
  led_amarillo(5,1000); 
  esp_deep_sleep_start();  
}
void loop_edited() {

  switch (state) {
  case INIT:
    if(!revisar_bat()){
      bootCount = 0;
      break;
    }
    init_state();
    break;
  case DATOS:
    Serial.print(F("Ciclo Numero "));
    Serial.println(ciclo);
    if(!revisar_bat()){
      break;
    }
    Serial.print(F("Ciclo Numero "));
    Serial.println(ciclo);
    tomaDatos(); //Tomar datos de las variables de interes
    Serial.print(F("Ciclo Numero "));
    Serial.println(ciclo);
    if(ciclo >= num_ciclos){
      Serial.println(F("IN pre envio"));
      state = ENVIO;
      merge_data_rtc(maxRecords + 1);
      eliminarUltimoDigitoURL();
      ciclo = 0;
    }else{
      ciclo ++;
      state = DORMIR;
    }
    last_state = DATOS;
    break;
  case ENVIO:
    Serial.println(F("Envio"));
    if(!revisar_bat()){
      break;
    }
    if(sendGet(msg,false)==false){
      Serial.println(F("ERROR ENVIO"));
      state = RESET_SIM;
      led_rojo(10, 500);
//      break;
    }
    remove_msg();
    update_time();
    bootCount = 1;
    cont_rst_sim = 0;    
    state = DORMIR; 
    break;
  case DORMIR:
    Serial.println(F("Sleep"));
    go_sleep();
    break; 
  default:
    // statements
    break;
  }
  //last_state = state;
}
bool revisar_bat(){
  bool resp = true;
  updateVolt();
  if(vol_bat <= MIN_VOLT_BAT and vol_panel <= MIN_VOLT_PAN){
    Serial.println(F("LOW BATTERY"));
    state = DORMIR;
    remove_msg();
    resp = false;
    led_rojo(5,1000);
  }
  return resp;
}
void update_time(){

  led_rojo_amarillo(10,500);

  if(sendGet(URL2,true)==false){
      Serial.println(F("ERROR ENVIO"));
      time_str = "10,10,10,10";
      led_rojo(15, 500);
      led_rojo_amarillo(15,500);
    }else{
      led_verde(30,300);
    }
  
 

  Serial.println(time_str);
  Serial.println("Init Time");

  time_str.remove(0,1);
  Serial.println(time_str);

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

  for(int n = 0; n < index; n++)
 { 
  Serial.println(strings[n]);
 }

 String hr,mn,dd,mm;
 hr = strings[2];
 mn = strings[3];
 dd = strings[0];
 mm = strings[1];
 
 Serial.println("Hora  " + String(dd) + "." + String(mm) + "  " + String(hr) + ":" + String(mn) );


  int dayy = dd.toInt();
  int monthh = mm.toInt();
  int hrr = hr.toInt();
  int minn = mn.toInt();
  Serial.println("Fecha  " + String(monthh) + "." + String(dayy));

  rtc.setTime(30, minn, hrr, dayy, monthh, 2021);  // 17th Jan 2021 15:24:30

  Serial.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));   // (String) returns time with specified format 

}

void remove_msg(){
  //Serial.println(F("REMOVE MSG"));
  ciclo = 0;
  delay(200);
  msg.remove(0,MAX_BYTES); //Se elimina el mensaje enviado
  msg.reserve(MAX_BYTES); //Se vuelve a disponer 290bytes de memoria para el nuevo mensaje

  init_RTC_memory();
  
  delay(1000);    
}
void init_RTC_memory(){
   for(int i = 0; i < maxRecords ;i++){
    Records[i].Dia = 0;
    Records[i].Mes = 0;
    Records[i].Hora = 0;
    Records[i].Minuto = 0;
  
    Records[i].Dir = 0;
    Records[i].Vel = 0;
    Records[i].Pluv = 0;
    Records[i].Temp = 0;
    Records[i].Hum = 0;
  
    Records[i].Panel = 0;
    Records[i].Bateria = 0;
  } 
}
void inicializarPines() {
  //Declarar pines de la señal de los sensores efecto hall como inputs
  //pinMode(interruptPin, INPUT_PULLUP);
  pinMode(transistor_sen, OUTPUT);
  pinMode(transistor_sim, OUTPUT);
  pinMode(transistor_aux, OUTPUT);


}
  int adc = 0;
  long adc_sum = 0;
  int adc2 = 0;
  long adc2_sum = 0; 
  
void updateVolt(){
  
  adc = 0;
  adc_sum = 0; 
  adc2 = 0;
  adc2_sum = 0; 

  for( int i = 0; i < 10; i++ ) 
  {
    adc = adc1_get_raw(ADC1_CHANNEL_0); 
    adc_sum = adc_sum + adc; 
//    Serial.println(i);
//    Serial.println(adc);
  }
  delay(500);

  for( int i = 0; i < 10; i++ ) 
  {
    adc2 = adc1_get_raw(ADC1_CHANNEL_3); 
    adc2_sum = adc2_sum + adc2; 
//    Serial.println(i);
//    Serial.println(adc2);
  }
 
// divide by the number of readings to get the average 
  adc = adc_sum / 10; 
  Serial.print("Promedio:  "); Serial.println(adc); 
  vol_bat = ((-0.0016 * adc) + 7)*100;

  adc2 = adc2_sum / 10; 
  Serial.print("Promedio:  "); Serial.println(adc2); 
  vol_panel = ((-0.0008 * adc2) + 3.5)*4.93*100;

   
  Serial.println(F("Update Voltage"));  
  Serial.println(String(vol_bat) + " - " + String(vol_panel));  

}
int direccion = 0;
int tics_vel = 0;
int tics_pluv = 0;

void tomaDatos() {
  led_verde_on();
  delay(3000);

  digitalWrite(transistor_sen, HIGH); //Habilitar nodo 5V de alimentacion sensores
  delay(100);
  digitalWrite(transistor_aux, HIGH); //Habilitar nodo 5V de alimentacion sensores
  delay(100);
  
  temp_hum(); //Ejecutar funcion para el sensado de temperatura y humedad
  hall(); //Ejecutar funcion para el sensado con los efecto hall (pluviografo y anemometro)

  digitalWrite(transistor_sen, LOW); //Desabilitar nodo 5V de alimentacion sensores
  delay(100);

  
  
  viento(); //Ejecutar funcion para el sensado con los efecto hall (pluviografo y anemometro)

  digitalWrite(transistor_aux, LOW); //Desabilitar nodo 5V de alimentacion sensores
  delay(100);
  
  
  updateVolt();
  Serial.println("BEFORE");
  
  Records[ciclo].Dia = rtc.getDay();
  Records[ciclo].Mes = rtc.getMonth();
  Records[ciclo].Hora = rtc.getHour(true);
  Records[ciclo].Minuto = rtc.getMinute();

  Records[ciclo].Dir = direccion;
  Records[ciclo].Vel = hallCounter1;
  Records[ciclo].Pluv = hallCounter;
  Records[ciclo].Temp = int(temp);
  Records[ciclo].Hum = int(h);

  Records[ciclo].Panel = vol_panel;
  Records[ciclo].Bateria = vol_bat;

  
  Serial.println("AFTER");
  Serial.println(Records[ciclo].Bateria);
  
  led_off();  

}
int val_dir_1 = 0;
int val_dir_2 = 0;
int val_vel_1 = 0;
int val_vel_2 = 0;

void viento(){
  delay(5000);
  val_dir_1 = 0;
  val_dir_2 = 0;
  val_vel_1 = 0;
  val_vel_2 = 0;  
  
  if (pcf8574.digitalRead(P0)==LOW){ 
    if(val_dir_1 != 0){
      val_dir_2 = 1; 
    }else {
      val_dir_1 = 1;
    }
  }
  if (pcf8574.digitalRead(P1)==LOW){ 
    if(val_dir_1 != 0){
      val_dir_2 = 2; 
    }else{
      val_dir_1 = 2;
    }
  }
  if (pcf8574.digitalRead(P2)==LOW){ 
    if(val_dir_1 != 0){
      val_dir_2 = 3; 
    }else{
      val_dir_1 = 3;
    }  }
  if (pcf8574.digitalRead(P3)==LOW){ 
    if(val_dir_1 != 0){
      val_dir_2 = 4; 
    }else{
      val_dir_1 = 4;
    }  }
  if (pcf8574.digitalRead(P4)==LOW){ 
    if(val_dir_1 != 0){
      val_dir_2 = 5; 
    }else{
      val_dir_1 = 5;
    }  }
  if (pcf8574.digitalRead(P5)==LOW){ 
    if(val_dir_1 != 0){
      val_dir_2 = 6; 
    }else{
      val_dir_1 = 6;
    }  }
  if (pcf8574.digitalRead(P6)==LOW){ 
     if(val_dir_1 != 0){
      val_dir_2 = 7; 
    }else{
      val_dir_1 = 7;
    }
  }
  if (pcf8574.digitalRead(P7)==LOW){ 
    if(val_dir_1 != 0){
      val_dir_2 = 8; 
    }else{
      val_dir_1 = 8;
    }
  }

  delay(50);
  Serial.print("Direccion -> ");
  Serial.println(String(val_dir_1) + " - " + String(val_dir_2));
  direccion = val_dir_1;
  delay(50);
}
void hall() {
  hallCounter = 0;//Pluviografo
  hallCounter1 = 0;//Vel
  starttime = millis();
  new_endtime = millis();
  //Serial.println(F("Init Hall"));
  unsigned long value = new_endtime - starttime;
  
  while ( value <= tiempo_hall) { //Realizar este loop durante 45 segundos
//    Serial.println((new_endtime - starttime));
//    Serial.println(F("Inside"));
    hallState = digitalRead(HALL_PLUVI); //Estado hall anemometro
    hallState1 = digitalRead(hall1_vel); //Estado hall pluviografo 1

    //Si hay un cambio en el estado HIGH ->LOW o LOW->HIGH en la señal de salida de los efecto hall
    //se aumenta en 1 el numero de tic tocs
    if (lLLastHall != lLastHall) { 
      if (hallState == HIGH and lastHall == HIGH and lLastHall == HIGH) {
        hallCounter++;
        Serial.println(hallCounter);
      }
      delay(10);
    }
    if (lLLastHall1 != lLastHall1) {
      if (hallState1 == HIGH and lastHall1 == HIGH and lLastHall1 == HIGH) {
        hallCounter1++;
        Serial.println(hallCounter1);
      }
      delay(10);
    }
    
    lLLastHall = lLastHall;
    lLastHall = lastHall;
    lastHall = hallState;

    lLLastHall1 = lLastHall1;
    lLastHall1 = lastHall1;
    lastHall1 = hallState1;

    
    new_endtime = millis();   
    value = new_endtime - starttime;
  }
  Serial.println("Counter"); 
  Serial.println(hallCounter);
  Serial.println(hallCounter1);  
  
}
void temp_hum() {
  delay(10000);
  h = uint8_t(dht.readHumidity()); //Leer la humedad relativa
  temp = uint16_t(10 * dht.readTemperature()); //Leer temperatura en grados Celsius
  Serial.println("Datos Ambiente");
  Serial.println(String(h) + " - " + String(temp));
}

void merge_data_rtc(int veces){
  msg.remove(0,MAX_BYTES); //Se elimina el mensaje enviado
  msg.reserve(MAX_BYTES); //Se vuelve a disponer 290bytes de memoria para el nuevo mensaje
  
  for (int i = 0; i < veces; i++){   
    finalString.reserve(40); //Se vuelve a disponer 40bytes de memoria para el nuevo mensaje
    finalString = String(Records[i].Dia) + "," + String(Records[i].Mes) + "," + String(Records[i].Hora) + "," + String(Records[i].Minuto) + "," + String(Records[i].Dir) + "," + String(Records[i].Vel) + "," + String(Records[i].Pluv) + "," + String(Records[i].Temp) + "," + String(Records[i].Hum) + String(Records[i].Panel) + "," + String(Records[i].Bateria)+ ";";
    Serial.println(finalString);
    msg = msg + finalString; //Guardar la toma de diferentes ciclos en una variable que sera enviada al servidor
    finalString.remove(0,40); //Se elimina el mensaje enviado
  }
  Serial.println(msg);
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
void resetSim800(){
  digitalWrite(transistor_sim, HIGH); //Habilitar nodo 5V de alimentacion sensores
  delay(5000);  
}
bool sendGet(String URL_param, boolean init_time){

   init_sim800();
  
//  Serial.println("--- SIM ON ----");
  led_amarillo_on();
  digitalWrite(transistor_sim, HIGH); //Habilitar nodo 5V de alimentacion sensores
  delay(3000);

  setupModule();
  delay(1500);
  
  // Establish GPRS connectivity (5 trials)
  bool sent = false;
  bool connected = false;
  for(uint8_t i = 0; i < 3 && !connected; i++) {
    delay(1500);
    connected = sim800l->connectGPRS();
  }

  // Check if connected, if not reset the module and setup the config again
  if(connected) {
    Serial.print(F("GPRS connected with IP "));
    Serial.println(sim800l->getIP());
  } else {
    Serial.println(F("GPRS not connected !"));
//    Serial.println(F("Reset the module."));
//    resetSim800();
//    setupModule();
//    return false;
  }

  Serial.println(F("Start HTTP GET..."));

  // Do HTTP GET communication with 10s for the timeout (read)
  uint16_t rc = sim800l->doGet(URL_param.c_str(), 10000);
   if(rc == 200 or rc == 409) {
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
  led_off();
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
  int cont_network = 0;
  NetworkRegistration network = sim800l->getRegistrationStatus();
  while(network != REGISTERED_HOME && network != REGISTERED_ROAMING && cont_network < 2) {
    delay(1000);
    network = sim800l->getRegistrationStatus();
    cont_network++;
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
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void disableWiFi(){
//    adc_power_off();
    WiFi.disconnect(true);  // Disconnect from the network
    WiFi.mode(WIFI_OFF);    // Switch WiFi off
    Serial.println("");
    Serial.println("WiFi disconnected!");
}
void disableBluetooth(){
    // Quite unusefully, no relevable power consumption
    btStop();
    Serial.println("");
    Serial.println("Bluetooth stop!");
}
 
void setModemSleep() {
    disableWiFi();
    disableBluetooth();
    setCpuFrequencyMhz(80);
    // Use this if 40Mhz is not supported
    // setCpuFrequencyMhz(80);
}
void initRGB(){
  ledcAttachPin(ledR, 1); // assign RGB led pins to channels
  ledcAttachPin(ledG, 2);
  ledcAttachPin(ledB, 3);
  
  // Initialize channels 
  // channels 0-15, resolution 1-16 bits, freq limits depend on resolution
  // ledcSetup(uint8_t channel, uint32_t freq, uint8_t resolution_bits);
  ledcSetup(1, 12000, 8); // 12 kHz PWM, 8-bit resolution
  ledcSetup(2, 12000, 8);
  ledcSetup(3, 12000, 8);
}
void led_off(){
  led_color(256,256,256);  
}
void led_amarillo_on(){
  led_color(0,0,255);
}
void led_rojo_on(){
  led_color(0,255,255);
}
void led_verde_on(){
  led_color(255,0,255);
}
void led_rojo_amarillo(int n, int t){
  for(int i = 0; i < n; i++){
    led_color(255,255,0);
    delay(t);
    led_color(0,0,255);
    delay(t);  
  }
}
void led_amarillo(int n, int t){
  for(int i = 0; i < n; i++){
    led_color(255,255,0);
    delay(t);
    led_color(256,256,256);
    delay(t);  
  }
}
void led_rojo(int n, int t){
  for(int i = 0; i < n; i++){
    led_color(0,0,255);
    delay(t);
    led_color(256,256,256);
    delay(t);  
  }
}
void led_verde(int n, int t){
  for(int i = 0; i < n; i++){
    led_color(255,0,255);
    delay(t);
    led_color(256,256,256);
    delay(t);  
  }
}
void led_azul(int n, int t){
  for(int i = 0; i < n; i++){
    led_color(255,255,0);
    delay(t);
    led_color(256,256,256);
    delay(t);  
  }
}
void led_color(int r, int g, int b){
  ledcWrite(1, r);
  ledcWrite(2, g);
  ledcWrite(3, b);
  delay(50); 
}
