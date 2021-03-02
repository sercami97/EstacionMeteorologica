#include "Arduino.h"
#include <Wire.h>
#include <SoftwareSerial.h>
#include "DHT.h" // Libreria DHT de Adafruit para DHT 11 o DHT 22 https://github.com/adafruit/DHT-sensor-library/archive/master.zip
#include <avr/sleep.h> //Libreria avr que contiene los metodos que controlan el modo sleep
#include <DS3232RTC.h>  //Libreria RTC https://github.com/JChristensen/DS3232RTC

#include "SIM800L.h"

#define SIM800_RX_PIN 5
#define SIM800_TX_PIN 4
#define SIM800_RST_PIN 10

const char APN[] = "internet.comcel.com.co";
const char URL[] = "http://eco.agromakers.org/api/v1/sensor/reporte_varios?id=2020080427&tramo=25,2,9,17,-2,377,1,9,1,0";


SIM800L* sim800l;

//--------------------DHT--------------
#define DHTPIN 7     //Pin donde está conectado el sensor DHT
#define DHTTYPE DHT22   //Sensor DHT22
DHT dht(DHTPIN, DHTTYPE);
float h;  //Variable de humedad relativa (0-100)
float temp; //Variable temeperatura grados centigrados

//------------------HALL----------------
#define  hall_an 3  //Pin Anemometro (Vel. Viento)
#define  hall_pluv1 8 //Pin Anemometro (Pluviografo1)

int hallCounter = 0;   //Contador tic tocs anemometro
int hallState = 0;         //Estado actual del sensor efecto hall anemometro (HIGH,LOW)
int lastHallState = 0;     //Estado previo del sensor efecto hall anemometro (HIGH,LOW)
int hallCounter1 = 0;   //Contador tic tocs ´pluviografo
int hallState1 = 0;      //Estado actual del sensor efecto hall pluviografo (HIGH,LOW)
int lastHallState1 = 0;    //Estado previo del sensor efecto hall pluviografo (HIGH,LOW)

//Variables 
//Variables de tiempo para el sensado del anemometro y el pluviografo
float starttime;
float new_endtime = 0;
int len;

//Numero de tomas y Sleep Mode
#define transistor_Pin 9   //Pin del transistor
#define interruptPin 2 //Pin de interrupcion para despertar el arduino
const int time_interval = 30; //Intervalo de tiempo para la toma de datos
int ciclo = 0; 
const int num_ciclos = 4; //Definir numero de tomas previas al envío (tomar 9 como valor máximo para evitar problemas de inestabilidad)


int last_wake_up_min; //Variable de tiempo en el que se despierta el arduino
int last_wake_up_hour; //Variable de tiempo en el que se despierta el arduino

int wake_up_min; //Variable de tiempo en el que se despierta el arduino
int wake_up_hour; //Variable de tiempo en el que se despierta el arduino


bool date_done = true;

int tiempo_hall = 3000;

// Infrarojo
int WV;
int direccion; //Variable de direccion como entero, tomara valores (1-9)

//Outputs
//Strings en donde se almacenan los datos tomados
String finalString;

String finalString_first;
String msg;

//Estado
int state = 1;

const int DORMIR = 2;
const int DATOS = 1;
const int ENVIO = 3;
const int WAIT = 4;

#define DTRpin 6

volatile boolean wait = false;

const float MIN_VOLT_BAT = 3.3;
const float MIN_VOLT_PAN = 8;

String response_at = "";

boolean first_init = true;
boolean first_run = true;

float vol_bat;
float vol_panel; 

void setup() {
  delay(5000);
  //Wire.begin();
  Serial.begin(9600);
  Serial.println("Setuuuup");
  msg.reserve(290); //Reservar memoria en bytes para la cadena de caracteres de los datos
  delay(1000);

  // Initialize a SoftwareSerial
  SoftwareSerial* serial = new SoftwareSerial(SIM800_RX_PIN, SIM800_TX_PIN);
  serial->begin(9600);
  delay(1000);
   
  // Initialize SIM800L driver with an internal buffer of 200 bytes and a reception buffer of 512 bytes, debug disabled
  //sim800l = new SIM800L((Stream *)serial, SIM800_RST_PIN, 350, 256);
  sim800l = new SIM800L((Stream *)serial, SIM800_RST_PIN, 256, 256, (Stream *)&Serial);

  
  inicializarPines(); //Inicializar pines sensores efecto hall, dht, interrupt, transistor
  inicializarSM(); //Inicializar Sleep Mode

  tomaDatos();

  messageServerGet(); //Envío del mensaje en el servidor
  msg.remove(0,290); //Se elimina el mensaje enviado
  msg.reserve(290); //Se vuelve a disponer 290bytes de memoria para el nuevo mensaje

  first_init = true;
  first_run = true;

  Serial.println("Setuuuup FIN");
}

void inicializarPines() {
  //Declarar pines de la señal de los sensores efecto hall como inputs
  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(transistor_Pin, OUTPUT);

  pinMode(DTRpin, OUTPUT);
  digitalWrite(DTRpin, LOW);
  
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

void loop() {

  switch (state) {
  case DATOS:
    tomaDatos(); //Tomar datos de las variables de interes
    if(ciclo >= num_ciclos){
      state = ENVIO;
      ciclo = 0;
    }else{
      ciclo ++;
      state = DORMIR;
    }
    break;
  case ENVIO:
    
    messageServerGet(); //Envío del mensaje en el servidor
    msg.remove(0,290); //Se elimina el mensaje enviado
    msg.reserve(290); //Se vuelve a disponer 290bytes de memoria para el nuevo mensaje  
    delay(1000);    
    state = DORMIR; 
    break;
  case DORMIR:
    Going_To_Sleep();
    updateVolt();
    if(vol_bat <= MIN_VOLT_BAT and vol_panel <= MIN_VOLT_PAN){
      state = DORMIR;
    }else{
      state = DATOS;
    }
    break;
  case WAIT: 
    state = DORMIR;
    break;
    
  default:
    // statements
    break;
}
  
}
void updateVolt(){
    vol_bat = analogRead(A1)*(0.0049)*2;
    vol_panel = analogRead(A2)*(4.73)*(0.0049);  
}

void Going_To_Sleep() {
    Serial.println("GOINGTOSLEEP");

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
    Serial.println(String(hour(t))+ "," + String(minute(t)));

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
  
      set_sleep_mode(SLEEP_MODE_PWR_DOWN);//Modo full sleep
      delay(1000); 
      sleep_cpu();//Activar sleep mode
      
      //Next Line after wakeup
      Serial.println("GOINGTOSLEEP_AWAKE");
      delay(1000);
      time_t t1 = RTC.get();
      Serial.println(String(hour(t1))+ "," + String(minute(t1)));
      
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

void tomaDatos() {
  digitalWrite(transistor_Pin, HIGH); //Habilitar nodo 5V de alimentacion sensores
  delay(100);
  
  Serial.println("Comienza Datos");
  hallCounter = 0;
  hallCounter1 = 0;
  hall(); //Ejecutar funcion para el sensado con los efecto hall (pluviografo y anemometro)
  temp_hum(); //Ejecutar funcion para el sensado de temperatura y humedad

  dir(); //Ejecutar funcion para el sensado de la direccion  
  delay(1000);
  time_t tiempo = RTC.get(); 
  //Guardar datos en un String
  updateVolt();
  Serial.println("Voltage Reading");
  Serial.println(String(int(vol_panel)) + " - " + String(int(vol_bat)));
  
  //finalString = String(day(tiempo))+ "," + String(month(tiempo))+ "," + String(hour(tiempo))+ "," + String(minute(tiempo)) + "," + String(int(temp)) + "," + String(int(h)) + "," + String(hallCounter) + "," + String(direccion) + "," + String(hallCounter1) +  ";";
  finalString = String(day(tiempo))+ "," + String(month(tiempo))+ "," + String(hour(tiempo))+ "," + String(minute(tiempo)) + "," + String(int(temp)) + "," + String(int(vol_bat)) + "," + "1" + "," + String(direccion) + "," + "1" + "," + String(int(vol_panel)) +  ";";
  
  if(first_run){
    finalString_first = String(day(tiempo))+ "," + String(month(tiempo))+ "," + String(hour(tiempo))+ "," + String(minute(tiempo)) + "," + "-1" + "," + String(int(vol_bat)) + "," + "1" + "," + String(direccion) + "," + "1" + "," + String(int(vol_panel/2)) +  ";";
    finalString = finalString_first;
    first_run = false;
  }  
  msg = msg + finalString; //Guardar la toma de diferentes ciclos en una variable que sera enviada al servidor
  Serial.println(msg);
  
  digitalWrite(transistor_Pin, LOW); //Desabilitar nodo 5V de alimentacion sensores
  delay(100);
}

void hall() {
  starttime = millis();
  Serial.print("Inicio Hall");
  //Serial.println(starttime);
  while ((new_endtime - starttime) < tiempo_hall) { //Realizar este loop durante 45 segundos
    hallState = digitalRead(hall_an); //Estado hall anemometro
    hallState1 = digitalRead(hall_pluv1); //Estado hall pluviografo 1
    
    //Si hay un cambio en el estado HIGH ->LOW o LOW->HIGH en la señal de salida de los efecto hall
    //se aumenta en 1 el numero de tic tocs
    if (hallState != lastHallState) { 
      if (hallState == HIGH) {
        hallCounter ++;
        Serial.println("HALL");
      }
      delay(50);
    }
    if (hallState1 != lastHallState1) {
      if (hallState1 == HIGH) {
        hallCounter1 ++;
        Serial.println("HALL1");
      }
      delay(50);
    }
    lastHallState = hallState;
    lastHallState1 = hallState1;
    new_endtime = millis();
  }
}

void temp_hum() {
  h = 10 * dht.readHumidity(); //Leer la humedad relativa
  temp = 10 * dht.readTemperature(); //Leer temperatura en grados Celsius
  Serial.println("Datos Ambiente");
  Serial.println(String(h) + " - " + String(temp));
}

void dir(){

  //Dependiendo del valor analogo obtenido de la señal del sensor infrarrojo se establece la direccion
    WV = analogRead(A3);
    Serial.println("DIR ->");
    //Serial.println(WV);

    if (WV > 145 && WV < 165){
      direccion = 1;
    }
    else if (WV >= 165 && WV < 175){
      direccion = 2;
    }
    else if (WV >= 175 && WV < 195){
      direccion = 3;
    }
    else if (WV >= 195 && WV < 225){
      direccion = 4;
    }
    else if (WV >= 225 && WV < 260){
      direccion = 5;
    }  
    else if (WV >= 260 && WV < 300){
      direccion = 6;
    }
    else if (WV >= 300 && WV < 325){
      direccion = 7;
    }
    else if (WV >= 325 && WV < 413){
      direccion = 8;
    }
    else{
      direccion = 9;
    }
    Serial.println(direccion);
}

void messageServerGet()
{
    Serial.println("Envio Datos");
    
    len = msg.length();
    msg.remove(len-1);
    Serial.println(msg);
    delay(2000);
    boolean retry = true;
    bool connected = false;
    int cont_error = 0;
    while(retry and cont_error < 5){

      connected = false;
      initModuleSIM();

      bool disconnected2 = sim800l->disconnectGPRS();
      for(uint8_t i = 0; i < 5 && !connected; i++) {
        delay(1000);
        disconnected2 = sim800l->disconnectGPRS();
      }
      
//      if(disconnected2) {
//        Serial.println(F("GPRS disconnected !"));
//      } else {
//        Serial.println(F("GPRS still connected !"));
//      }
      
      // Establish GPRS connectivity (5 trials)
      
      for(uint8_t i = 0; i < 5 && !connected; i++) {
        delay(1000);
        connected = sim800l->connectGPRS1();
      }
      // Establish GPRS connectivity (5 trials)
      // Check if connected, if not reset the module and setup the config again
//      if(connected) {
//        Serial.println(F("GPRS1 connected !"));
//      } else {
//        Serial.println(F("GPRS1 not connected !"));
//        Serial.println(F("Reset the module."));
//      }
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
        Serial.println(F("Reset the module."));
        
        //break;
        //reset_sim800l();

      }
    
      Serial.println(F("Start HTTP GET..."));

      connected = false;
      for(uint8_t i = 0; i < 5 && !connected; i++) {
        delay(1000);
        connected = sim800l->initiateHTTPINIT();
      }
    
//      // Check if connected, if not reset the module and setup the config again
//      if(connected) {
//        Serial.println(F("HTTP SUCCESS !"));
//      } else {
//        Serial.println(F("HTTP FAIL !"));
//        Serial.println(F("Reset the module."));
//      }
    
      // Do HTTP GET communication with 10s for the timeout (read)
//      String msg_fin = "http://eco.agromakers.org/api/v1/sensor/reporte_varios?id=2020080427&tramo="+msg;
//      Serial.println("Mensaje Final");
//      Serial.println(msg_fin);
//      Serial.println(msg_fin.length());
//      int str_len = msg_fin.length() + 1;
//      // Prepare the character array (the buffer) 
//      char char_array[str_len];
//      // Copy it over 
//      msg_fin.toCharArray(char_array, str_len);
      uint16_t rc = sim800l->doGet(URL, 15000);
       if(rc == 200) {
        // Success, output the data received on the serial
        Serial.print(F("HTTP GET successful ("));
        Serial.print(sim800l->getDataSizeReceived());
        Serial.println(F(" bytes)"));
        Serial.print(F("Received : "));
        Serial.println(sim800l->getDataReceived());
        retry = false;
        break;
      } else {
        // Failed...
        Serial.print(F("HTTP GET error "));
        Serial.println(rc);
      }
      //reset_sim800l();
      cont_error++;
      
      // Close GPRS connectivity (5 trials)
      bool disconnected = sim800l->disconnectGPRS();
      for(uint8_t i = 0; i < 5 && !connected; i++) {
        delay(1000);
        disconnected = sim800l->disconnectGPRS();
      }
      
//      if(disconnected) {
//        Serial.println(F("GPRS disconnected !"));
//      } else {
//        Serial.println(F("GPRS still connected !"));
//      }
//    
      // Go into low power mode
      bool lowPowerMode = sim800l->setPowerMode(SLEEP);
//      if(lowPowerMode) {
//        Serial.println(F("Module in low power mode"));
//      } else {
//        Serial.println(F("Failed to switch module to low power mode"));
//      }
//      sim800l->reset();
    }
}

//void reset_sim800l(){
//  // Go into low power mode
//  bool lowPowerMode = sim800l->setPowerMode(SLEEP);
//  if(lowPowerMode) {
//    Serial.println(F("Module in low power mode"));
//  } else {
//    Serial.println(F("Failed to switch module to low power mode"));
//  }
//  delay(30000);
//  bool nornmalMode = sim800l->setPowerMode(NORMAL);
//  if(nornmalMode) {
//    Serial.println(F("Module in normal power mode"));
//  } else {
//    Serial.println(F("Failed to switch module to normal power mode"));
//  }
//}
void initModuleSIM(){
  // Wait until the module is ready to accept AT commands
  while(!sim800l->isReady()) {
    Serial.println(F("Problem to initialize AT command, retry in 1 sec"));
    delay(1000);
  }
  while(!sim800l->isReadyClk()) {
    Serial.println(F("Problem to initialize AT command CLOCK, retry in 1 sec"));
    delay(1000);
  }
  Serial.println(F("Setup Complete!"));
  bool nornmalMode = sim800l->setPowerMode(NORMAL);
//  if(nornmalMode) {
//    Serial.println(F("Module in normal power mode"));
//  } else {
//    Serial.println(F("Failed to switch module to normal power mode"));
//  }
  // Wait for the GSM signal
  uint8_t signal = sim800l->getSignal();
  while(signal <= 0) {
    delay(1000);
    signal = sim800l->getSignal();
    Serial.print(signal);
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
