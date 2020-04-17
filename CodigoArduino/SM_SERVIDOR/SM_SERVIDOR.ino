#include "Arduino.h"
#include <Wire.h>
#include <SoftwareSerial.h>
#include "DHT.h" // Libreria DHT de Adafruit para DHT 11 o DHT 22 https://github.com/adafruit/DHT-sensor-library/archive/master.zip
#include <avr/sleep.h> //Libreria avr que contiene los metodos que controlan el modo sleep
#include <DS3232RTC.h>  //Libreria RTC https://github.com/JChristensen/DS3232RTC

//VERSION COMENTADA
//Sleep Mode
#define transistor_Pin 9   //Pin del transistor
#define interruptPin 2 //Pin de interrupcion para despertar el arduino
const int time_interval = 30; //Intervalo de tiempo para la toma de datos

//DHT
#define DHTPIN 7     //Pin donde está conectado el sensor DHT
#define DHTTYPE DHT22   //Sensor DHT22
DHT dht(DHTPIN, DHTTYPE);
float h;  //Variable de humedad relativa (0-100)
float t; //Variable temeperatura grados centigrados

//HALL
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
//Numero de ciclos de sensado
int ciclo = 0;
const int num_ciclos = 10;
int wake_up_min; //Variable de tiempo en el que se despierta el arduino

//SIM
SoftwareSerial mySerial(5, 4);//TX,RX
String _buffer;

// Infrarojo
int WV;
int direccion; //Variable de direccion como entero, tomara valores (1-9)

//Outputs
//Strings en donde se almacenan los datos tomados
String finalString;
String msg;


void setup() {
  delay(2000); 
  Serial.begin(9600);
  inicializar(); //Inicializar pines sensores efecto hall y dht
  inicializarSM(); //Inicializar Sleep Mode
  _buffer.reserve(50); 
  msg.reserve(290); //Reservar memoria en bytes para la cadena de caracteres de los datos
  mySerial.begin(9600);
  delay(1000);
}

void inicializar() {
  //Declarar pines de la señal de los sensores efecto hall como inputs
  pinMode(hall_an, INPUT); 
  pinMode(hall_pluv1, INPUT);
  //Inicializar el DHT
  dht.begin();
}

void inicializarSM() {
  //Declarar pines necesarios para el funcionamiento del sleep mode
  pinMode(LED_BUILTIN, OUTPUT);  //LED arduino usado para conocer el estado del arduino
  pinMode(interruptPin, INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(transistor_Pin, OUTPUT);
  pinMode(transistor_Pin, HIGH);

  RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
  RTC.alarm(ALARM_1);
  RTC.alarmInterrupt(ALARM_1, false);
  RTC.squareWave(SQWAVE_NONE); 

  time_t t; //Crear variable temporal de tiempo
  t = RTC.get(); //Obtener el tiempo actual del RTC
  wake_up_min = minute(t) + time_interval;
  //Condicional necesario para cambio de horas
  if (wake_up_min >= 60){
    wake_up_min = wake_up_min - 60;
  }
  RTC.setAlarm(ALM1_MATCH_MINUTES , 0, wake_up_min, 0, 0); //Poner alarma
  RTC.alarm(ALARM_1); //Eliminar bandera de la alarma
  RTC.squareWave(SQWAVE_NONE);
  RTC.alarmInterrupt(ALARM_1, true);  //Habilitar interrupcion de "ALARM_1"

}

void initModule(){
  //Comandos AT necesarios para GSM/GPRS 
  mySerial.println(F("AT+SAPBR=3,1,\"Contype\",\"GPRS\""));
  runSerial();
  delay(1500);
  mySerial.println(F("AT+SAPBR=3,1,\"APN\",\"internet.comcel.com.co\""));
  runSerial();
  delay(100);
  mySerial.println(F("AT+SAPBR=3,1,USER,\"comcel\""));
  runSerial();
  delay(100);
  mySerial.println(F("AT+SAPBR=3,1,PWD,\"comcel\""));
  runSerial();
  delay(100);
  mySerial.println(F("AT+SAPBR=1,1"));
  runSerial();
  delay(3500);
  mySerial.println(F("AT+SAPBR=2,1"));
  runSerial();
  delay(2500);
}

void loop() {
  //Loop principal, se realizan n = "num_ciclos" toma de datos antes de hacer 
  //post en el servidor y borrar los datos del intervalo de tiempo en cuestion
  while (ciclo < num_ciclos){
    Going_To_Sleep();
  }
  Serial.println(msg);
  messageServerPost();
  delay(60000);
  msg.remove(0,290);
  msg.reserve(290);
  delay(5000);
  ciclo = 0;
  inicializarSM();
}

void Going_To_Sleep() {
    digitalWrite(transistor_Pin, LOW);
    sleep_enable();//Habilitar sleep mode
    Serial.println("Sleep Activado");
    attachInterrupt(0, wakeUp, LOW); //Adjuntar la interrupcion al pin D2
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);//Modo full sleep
    digitalWrite(LED_BUILTIN, LOW); //Apagar LED arduino
    time_t t;
    t = RTC.get(); 
    Serial.println("Sleep  Time: " + String(hour(t)) + ":" + String(minute(t)) + ":" + String(second(t))); 
    delay(1000); 
    sleep_cpu();//Activar sleep mode
    Serial.println("Desperto");
    digitalWrite(LED_BUILTIN, HIGH); //Encender LED arduino
    digitalWrite(transistor_Pin, HIGH); //Habilitar nodo 5V de alimentacion sensores
    delay(5000);
    tomaDatos(); //Tomar datos de las variables de interes
    t = RTC.get();
    wake_up_min = minute(t) + time_interval;
    if (wake_up_min >= 60){
      wake_up_min =  wake_up_min - 60;
    }
    RTC.setAlarm(ALM1_MATCH_MINUTES , 0, wake_up_min, 0, 0); //Poner nueva alarma
    RTC.alarm(ALARM_1);
    ciclo ++; //Sumar un ciclo de sensado
}

void wakeUp() {
  Serial.println(F("Interrrupt Fired"));
  sleep_disable();//Deshabilitar sleep mode
  detachInterrupt(0); //Eliminar la interrupcion del pin 2;
}

void tomaDatos() {
  hallCounter = 0;
  hallCounter1 = 0;
  hall(); //Ejecutar funcion para el sensado con los efecto hall (pluviografo y anemometro)
  temp_hum(); //Ejecutar funcion para el sensado de temperatura y humedad
  dir(); //Ejecutar funcion para el sensado de la direccion
  time_t tiempo;
  tiempo = RTC.get(); 
  //Guardar datos en un String
  finalString = String(hour(tiempo))+ "," + String(minute(tiempo)) + "," + String(h) + "," + String(t) + "," + String(hallCounter) + "," + String(hallCounter1) + "," + String(direccion)+ ";";
  msg = msg + finalString; //Guardar la toma de diferentes ciclos en una variable que sera enviada al servidor
  Serial.println(msg);
}

void hall() {
  starttime = millis();
  Serial.println(starttime);
  while ((new_endtime - starttime) < 45000) { //Realizar este loop durante 45 segundos
    hallState = digitalRead(hall_an); //Estado hall anemometro
    hallState1 = digitalRead(hall_pluv1); //Estado hall pluviografo 1
    
    //Si hay un cambio en el estado HIGH ->LOW o LOW->HIGH en la señal de salida de los efecto hall
    //se aumenta en 1 el numero de tic tocs
    if (hallState != lastHallState) { 
      if (hallState == HIGH) {
        hallCounter ++;
      }
      delay(50);
    }
    if (hallState1 != lastHallState1) {
      if (hallState1 == HIGH) {
        hallCounter1 ++;
      }
      delay(50);
    }
    lastHallState = hallState;
    lastHallState1 = hallState1;
    new_endtime = millis();
  }
}

void temp_hum() {
  h = dht.readHumidity(); //Leer la humedad relativa
  t = dht.readTemperature(); //Leer temperatura en grados Celsius
}

void dir(){

  //Dependiendo del valor analogo obtenido de la señal del sensor infrarrojo se establece la direccion
    WV = analogRead(A3);

    if (WV > 230 && WV < 245){
      direccion = 1;
    }
    else if (WV >= 245 && WV < 255){
      direccion = 2;
    }
    else if (WV >= 255 && WV < 270){
      direccion = 3;
    }
    else if (WV >= 270 && WV < 285){
      direccion = 4;
    }
    else if (WV >= 285 && WV < 295){
      direccion = 5;
    }  
    else if (WV >= 295 && WV < 310){
      direccion = 6;
    }
    else if (WV >= 310 && WV < 343){
      direccion = 7;
    }
    else if (WV >= 310 && WV < 413){
      direccion = 8;
    }
    else{
      direccion = 9;
    }
}

void messageServerPost()
{
   initModule(); //Inicializacion del modulo
   //Comandos AT necesarios para postear el mensaje final en el servidor
   delay(5000);
   mySerial.println(F("AT+HTTPINIT\r"));
   runSerial();
   delay(500);
   mySerial.println(F("AT+HTTPPARA=\"CID\",1\r"));
   runSerial();
   delay(500);
   mySerial.println(F("AT+HTTPPARA=\"URL\",\"http://datos-env.iafjn3xg9q.us-east-1.elasticbeanstalk.com/datos/new/innovandes\"\r"));
   runSerial();
   delay(500);
   mySerial.println(F("AT+HTTPPARA=\"CONTENT\",\"application/json\""));
   runSerial();
   delay(500);
   mySerial.println(F("AT+HTTPDATA=1000,10000\r"));
   runSerial();
   delay(1000);
   mySerial.println("{\"name\": \"EM1\", \"description\": \""+ msg + "\"}");
   runSerial();
   delay(10000);
   mySerial.println(F("AT+HTTPACTION=1\r"));
   runSerial();
   delay(3000);
   mySerial.println(F("AT+HTTPREAD=0,100\r"));
   runSerial();
   delay(500);
   mySerial.println(F("AT+HTTPTERM\r"));
   runSerial();
   delay(3000);
}

void runSerial(){
    while (mySerial.available()){
      Serial.write(mySerial.read());  
    }
}
