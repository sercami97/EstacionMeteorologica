#include "Arduino.h"
#include "DHT.h" // Libreria DHT de Adafruit para DHT 11 o DHT 22 https://github.com/adafruit/DHT-sensor-library/archive/master.zip
#include <avr/sleep.h> //Libreria avr que contiene los metodos que controlan el modo sleep
#include <DS3232RTC.h>  //Libreria RTC https://github.com/JChristensen/DS3232RTC
#include "SIM800L.h"
//#include <MemoryUsage.h>
#include <MemoryFree.h>

const char APN[] = "hologram";
//const char USR[] = "comcel";
const char URL[] = "http://eco.agromakers.org/api/r_v?id=2020080427&tramo=";
const char URL2[] = "http://eco.agromakers.org/api/date";

SIM800L* sim800l;


//------------------------HALL---------------------
#define  hall_an 10  //Pin Anemometro (Vel. Viento)
#define  hall_pluv1 13 //Pin Anemometro (Pluviografo1)

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

unsigned long tiempo_hall = 15000;

//------------------ END HALL --------------

//---------- MOSFETs ---------------
#define transistor_sen 12   //Pin del transistor
#define transistor_sim 7   //Pin del transistor
#define transistor_aux 9   //Pin del transistor

//---------------- SLEEP MODE & RTC ---------------
//Variables 
//Variables de tiempo para el sensado del anemometro y el pluviografo
unsigned long starttime;
unsigned long new_endtime = 0;
int len;

//Numero de tomas y Sleep Mode
#define interruptPin 2 //Pin de interrupcion para despertar el arduino

const uint8_t time_interval = 1; //Intervalo de tiempo para la toma de datos
const uint8_t num_ciclos = 3; //Definir numero de tomas previas al envío (tomar 6 como valor máximo para evitar problemas de inestabilidad)

uint8_t ciclo = 0; 

uint8_t last_wake_up_min; //Variable de tiempo en el que se despierta el arduino
uint8_t last_wake_up_hour; //Variable de tiempo en el que se despierta el arduino

uint8_t wake_up_min; //Variable de tiempo en el que se despierta el arduino
uint8_t wake_up_hour; //Variable de tiempo en el que se despierta el arduino

String time_str = "";

//----------------END SLEEP MODE & RTC ---------------


// -------------------- IR -----------------
int WV;
//uint8_t direccion; //Variable de direccion como entero, tomara valores (1-9)

// -------------------- END IR -----------------

// --------------------- DHT ---------------------
#define DHTPIN 11     //Pin donde está conectado el sensor DHT
#define DHTTYPE DHT22   //Sensor DHT22
DHT dht(DHTPIN, DHTTYPE);
uint8_t h;  //Variable de humedad relativa (0-100)
uint16_t temp; //Variable temeperatura grados centigrados

// -------------------- END DHT ---------------

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

const uint16_t MIN_VOLT_BAT = 345;
const uint8_t MIN_VOLT_PAN = 80;

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

uint16_t vol_bat;
uint8_t vol_panel; 

uint8_t cont_rst_sim = 0;

#define RST_ARD 4

const uint16_t MAX_BYTES = 80;

// --------------END SYSTEM VARIABLES -------------


//Que se cuenten envios de rrores pero que siga los siguientes estados. Reset cuando hallan 3 mensajes que no se han enviado.
void setup() {
  // Initialize Serial Monitor for debugging
  Serial.begin(9600);
  while(!Serial);
  
  Serial.println("--- BEGIN SETUP ----"); 
  
  pinMode(RST_ARD, OUTPUT);
  delay(100); 
  digitalWrite(RST_ARD, LOW);
  delay(200);
  
//  usageMemory();
  
  inicializarPines(); //Inicializar pines sensores efecto hall, dht, interrupt, transistor
  inicializarSM(); //Inicializar Sleep Mode

  memory();
  

  Serial.println("--- SIM ON ----");
  digitalWrite(transistor_sim, HIGH); //Habilitar nodo 5V de alimentacion sensores
  delay(10000);

  sim800l = new SIM800L((Stream *)&Serial, NULL, MAX_BYTES, 20);

  Serial.println("--- SIM OFF ----");
  digitalWrite(transistor_sim, LOW); //Habilitar nodo 5V de alimentacion sensores
  delay(500);

//  usageMemory();
  memory();
  delay(500);
//  Serial.println("--- SIM OFF ----");
//  digitalWrite(transistor_sim, LOW); //Habilitar nodo 5V de alimentacion sensores
//  delay(500);


  msg.reserve(MAX_BYTES); //Reservar memoria en bytes para la cadena de caracteres de los datos
  delay(1000);
  memory();


  //delay(5000);
  
  //usageMemory();
  //tomaDatos();
  updateVolt();
  if(vol_bat <= MIN_VOLT_BAT and vol_panel <= MIN_VOLT_PAN){
    Serial.println(F("LOW BATTERY UNABLE TO SEND SIM"));
    state = DORMIR;
  }else{
    Serial.println(F("Init Time"));
    
    initTime();

    state = DATOS;
//    usageMemory();
    memory();
    tomaDatos();
    delay(1000);
    tomaDatos();
    memory();
//    usageMemory();
    //eliminarUltimoDigitoURL();

    Serial.println(F("Begin Send"));
    if(sendGet(msg,false)){
      state = DATOS;
      remove_msg();
    }else{
      state = RESET_SIM;
    }
  }
  

}
//void usageMemory(){
//    Serial.println(F("Starting state of the memory:"));
//    Serial.println();
//    
//    MEMORY_PRINT_START
//    MEMORY_PRINT_HEAPSTART
//    MEMORY_PRINT_HEAPEND
//    MEMORY_PRINT_STACKSTART
//    MEMORY_PRINT_END
//    MEMORY_PRINT_HEAPSIZE
//
//    Serial.println();
//    Serial.println();
//   
//    FREERAM_PRINT;
//}
void initTime(){
  
  sendGet(URL2,true);

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

  RTC.begin();
  delay(500);

  int dayy = dd.toInt();
  int monthh = mm.toInt();

  Serial.println("Fecha  " + String(monthh) + "." + String(dayy));

  
  tmElements_t tm;
  tm.Hour = hr.toInt();
  tm.Minute = mn.toInt();
  tm.Second = 00;
  tm.Day = dayy;
  tm.Month = monthh;
  tm.Year = 2021 - 1970;
  RTC.write(tm); 
  delay(1000);
  
  time_t t = RTC.get();
  Serial.println(F("Time Datos"));
  Serial.println((String(month(t)))+ "," + (String(day(t)))+ "," + (String(hour(t))) + "," + (String(minute(t))));

  Serial.println("END Time Init");
}
void memory(){
  Serial.print("freeMemory()=");
  Serial.println(freeMemory());
}
void loop() {

  switch (state) {
  case DATOS:
    //usageMemory();
    tomaDatos(); //Tomar datos de las variables de interes
    Serial.print(F("Ciclo Numero "));
    Serial.println(ciclo);
//    Serial.println(num_ciclos);
    if(ciclo >= num_ciclos){
      Serial.println(F("IN pre envio"));
      state = ENVIO;
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
    //usageMemory();
    updateVolt();
    if(vol_bat <= MIN_VOLT_BAT and vol_panel <= MIN_VOLT_PAN){
      Serial.println(F("LOW BATTERY UNABLE TO SEND SIM"));
      state = DORMIR;
      break;
    }
    if(sendGet(msg,false)==false){
      Serial.println(F("ERROR ENVIO"));
      state = RESET_SIM;
      break;
    }
    remove_msg();
    cont_rst_sim = 0;    
    state = DORMIR; 
    break;
  case DORMIR:
    Going_To_Sleep();
    state = ASK_BAT;
    break;
  case RESET_SIM:
    digitalWrite(transistor_sim, HIGH); //Habilitar nodo 5V de alimentacion sensores
    delay(5000);
    Serial.println(F("Reset SIM"));
    sim800l->reset();
    delay(5000);
    state = ASK_BAT;
    last_state = RESET_SIM;
    //cont_rst_sim = 0 -> Primer RESET SIM vuelve intentar envio
    //cont_rst_sim = 1 -> Segundo RESET SIM vuelve intentar envio
    //cont_rst_sim = 2 -> Tercer RESET SIM borra envio y vuelve a tomar datos.
    //cont_rst_sim = 3 -> Reset Arduino

    if(cont_rst_sim == 2){
      Serial.println(F("Begin DATA after reset"));
      state = DATOS;
      remove_msg();
      //Serial.println(ciclo);
    }
    if(cont_rst_sim >= 3){
      state = RESET_ARDU;
      break;
    }
    digitalWrite(transistor_sim, LOW); //Habilitar nodo 5V de alimentacion sensores
    delay(100);
    cont_rst_sim++;
    break;
  case RESET_ARDU:
    //Serial.println(F("Reset Arduino"));
    cont_rst_sim = 0;
    digitalWrite(RST_ARD,LOW);
    break;
  case ASK_BAT:
    //Serial.println(F("ASK BATTERY STATE"));
    updateVolt();
    if(vol_bat <= MIN_VOLT_BAT and vol_panel <= MIN_VOLT_PAN){
      //Serial.println(F("LOW BATTERY"));
      state = DORMIR;
      remove_msg();
      break;
    }
    if(last_state == RESET_SIM){
      state = ENVIO;
      last_state = ASK_BAT;
    }else{
      state = DATOS;
    }
    break;  
  default:
    // statements
    break;
  }
  //last_state = state;
}
void remove_msg(){
  //Serial.println(F("REMOVE MSG"));
  ciclo = 0;
  delay(200);
  msg.remove(0,MAX_BYTES); //Se elimina el mensaje enviado
  msg.reserve(MAX_BYTES); //Se vuelve a disponer 290bytes de memoria para el nuevo mensaje
  delay(1000);    
}
void inicializarPines() {
  //Declarar pines de la señal de los sensores efecto hall como inputs
  //pinMode(interruptPin, INPUT_PULLUP);
  pinMode(transistor_sen, OUTPUT);
  pinMode(transistor_sim, OUTPUT);

  //pinMode(DTRpin, OUTPUT);
  //digitalWrite(DTRpin, LOW);
  
  pinMode(hall_an, INPUT); 
  pinMode(hall_pluv1, INPUT);
  
  dht.begin();//Inicializar el DHT
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
void tomaDatos() {
  digitalWrite(transistor_sen, HIGH); //Habilitar nodo 5V de alimentacion sensores
  delay(100);
  
  //Serial.println(F("Comienza Datos"));
  hallCounter = 0;
  hallCounter1 = 0;
  hall(); //Ejecutar funcion para el sensado con los efecto hall (pluviografo y anemometro)
  temp_hum(); //Ejecutar funcion para el sensado de temperatura y humedad

  uint8_t contDifDir = 0;
  uint8_t dirF=0;
  boolean out = true;
  while(out and contDifDir<3){
    uint8_t dir1 = dir();
    delay(1000);
    uint8_t dir2 = dir();
    delay(1000);
    if(dir1 == dir2){
      dirF = dir1;
      out = false;;  
    }else{
      contDifDir++;
      dirF = dir1;
    }
  }  
  delay(1000);
  time_t tiempo = RTC.get();
  //Serial.println(F("Time Datos"));
  //Serial.println((String(hour(tiempo))+ "," + String(minute(tiempo))));
 
  //Guardar datos en un String
  updateVolt();

  finalString.reserve(40); //Se vuelve a disponer 290bytes de memoria para el nuevo mensaje
  finalString = String(day(tiempo))+ "," + String(month(tiempo))+ "," + String(hour(tiempo))+ "," + String(minute(tiempo)) + "," + String(temp) + "," + String(h) + "," + String(hallCounter) + "," + String(dirF) + "," + String(hallCounter1) + "," + String(vol_panel) + "," + String(vol_bat)+";";
  
  if(first_run){
    finalString = String(day(tiempo))+ "," + String(month(tiempo))+ "," + String(hour(tiempo))+ "," + String(minute(tiempo)) + "," + "-1" + "," + String(h) + "," + String(hallCounter) + "," + String(dirF) + "," + String(hallCounter1) + "," + String(vol_panel) + "," + String(vol_bat) + ";";
    first_run = false;
  }  
  msg = msg + finalString; //Guardar la toma de diferentes ciclos en una variable que sera enviada al servidor
  finalString.remove(0,40); //Se elimina el mensaje enviado
  //Serial.println(msg);
  
  digitalWrite(transistor_sen, LOW); //Desabilitar nodo 5V de alimentacion sensores
  delay(100);
}

void hall() {
  hallCounter = 0;
  hallCounter1 = 0;
  starttime = millis();
  new_endtime = millis();
  //Serial.println(F("Init Hall"));
  unsigned long value = new_endtime - starttime;
  
  while ( value <= tiempo_hall) { //Realizar este loop durante 45 segundos
//    Serial.println((new_endtime - starttime));
//    Serial.println(F("Inside"));
    hallState = digitalRead(hall_an); //Estado hall anemometro
    hallState1 = digitalRead(hall_pluv1); //Estado hall pluviografo 1
//    if (hallState == HIGH) {
//      Serial.println("HIGH");
//    }else{
//      Serial.println("LOW");
//    }
    //Si hay un cambio en el estado HIGH ->LOW o LOW->HIGH en la señal de salida de los efecto hall
    //se aumenta en 1 el numero de tic tocs
    if (lLLastHall != lLastHall) { 
      if (hallState == HIGH and lastHall == HIGH and lLastHall == HIGH) {
        hallCounter++;
        //Serial.println(hallCounter);
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
//  Serial.println("Counter"); 
//  Serial.println(hallCounter);
//  Serial.println(hallCounter1);  
  
}

void temp_hum() {
  h = uint8_t(dht.readHumidity()); //Leer la humedad relativa
  temp = uint16_t(10 * dht.readTemperature()); //Leer temperatura en grados Celsius
  //Serial.println("Datos Ambiente");
  //Serial.println(String(h) + " - " + String(temp));
}
uint8_t dir(){

  //Dependiendo del valor analogo obtenido de la señal del sensor infrarrojo se establece la direccion
    WV = analogRead(A3);
    //Serial.println("DIR ->");
    //Serial.println(WV);
    uint8_t direccion;

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

    //Serial.println(direccion);
    return direccion;
}
void Going_To_Sleep() {
    Serial.println(F("GOINGTOSLEEP"));

    RTC.begin();
    //Eliminar cualquier alarma previa
    RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
    RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
    RTC.alarm(ALARM_1);
    RTC.alarm(ALARM_2);
    RTC.alarmInterrupt(ALARM_1, false);
    RTC.alarmInterrupt(ALARM_2, false);
    
    RTC.squareWave(SQWAVE_NONE);
    
    time_t t = RTC.get();
    Serial.println((String(hour(t))+ "," + String(minute(t))));
    if(first_init == true){
      last_wake_up_min = minute(t);
      last_wake_up_hour = hour(t);
      first_init = false;
    }
    
    wake_up_min = last_wake_up_min + time_interval;
    wake_up_hour = last_wake_up_hour;
    if (wake_up_min >= 60){
      wake_up_min =  wake_up_min - 60;
      wake_up_hour += 1;
    }
    if(wake_up_hour == 24){
      wake_up_hour = 0;
    }
    last_wake_up_min = wake_up_min;
    last_wake_up_hour = wake_up_hour;
    
    bool incorrect_alarm = false;
    if(hour(t)==wake_up_hour){
        if(minute(t) >= wake_up_min){
          incorrect_alarm = true;
        }
      }
      else if(hour(t)>wake_up_hour){
        incorrect_alarm = true;
      }
    
    if(incorrect_alarm){
      Serial.println(F("Incorrect Alarm"));
      wake_up_min = minute(t) + time_interval;
      wake_up_hour = hour(t);
      if (wake_up_min >= 60){
        wake_up_min =  wake_up_min - 60;
        wake_up_hour += 1;
      }
      if(wake_up_hour == 24){
        wake_up_hour = 0;
      }
      last_wake_up_min = wake_up_min;
      last_wake_up_hour = wake_up_hour;
    }
    
    Serial.println(String(wake_up_hour) + " - " + String(wake_up_min));
    boolean correct_wakeup = false;
    while(correct_wakeup==false){
      RTC.setAlarm(ALM1_MATCH_HOURS , 0, wake_up_min, wake_up_hour, 0); //Poner nueva alarma
      RTC.alarm(ALARM_1);
      RTC.alarmInterrupt(ALARM_1, true);  //Habilitar interrupcion de "ALARM_1"
      
  
      pinMode(interruptPin, INPUT_PULLUP);
      delay(100);
      sleep_enable();//Habilitar sleep mode
      attachInterrupt(digitalPinToInterrupt(interruptPin), wakeUp, LOW); //Adjuntar la interrupcion al pin D2

      delay(1000); 

      set_sleep_mode(SLEEP_MODE_PWR_DOWN);//Modo full sleep
      delay(1000); 
      sleep_cpu();//Activar sleep mode
      
      //Next Line after wakeup
      Serial.println(F("GOINGTOSLEEP_AWAKE"));
      delay(1000);
      time_t t1 = RTC.get();
      Serial.println(String(hour(t1))+ " - " + String(minute(t1)));

      correct_wakeup = true;
      if(hour(t1)==wake_up_hour){
        if(minute(t1) >= wake_up_min){
          correct_wakeup = true;
        }
      }
    }
}

void wakeUp() {
  //Serial.println(F("Interrrupt Fired"));
  sleep_disable();//Deshabilitar sleep mode
  detachInterrupt(0); //Eliminar la interrupcion del pin 2;
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

  Serial.println("--- SIM ON ----");
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

  Serial.println("--- SIM OFF ----");
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

//bool sendGet(String msg_param, boolean init_time) {
//  Serial.println("--- SIM ON ----");
//  digitalWrite(transistor_sim, HIGH); //Habilitar nodo 5V de alimentacion sensores
//  delay(7000);
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
//        if(init_time){
//          time_str = sim800l->getDataReceived();  
//          
//        }
//        
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
//  Serial.println("--- SIM OFF ----");
//  digitalWrite(transistor_sim, LOW); //Habilitar nodo 5V de alimentacion sensores
//  delay(500);
//  return enviado;
//}
//
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
