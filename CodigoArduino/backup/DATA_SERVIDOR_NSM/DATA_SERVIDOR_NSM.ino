#include "DHT.h"  //Libreria DHT de Adafruit para DHT 11 o DHT 22 https://github.com/adafruit/DHT-sensor-library/archive/master.zip
#include "Arduino.h"
#include <Wire.h>
#include <SoftwareSerial.h>
#include <DS3232RTC.h>  //Libreria RTC https://github.com/JChristensen/DS3232RTC


//Sleep Mode
#define transistor_Pin 9   //Pin transistor

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
int counter = 0;

//Outputs
//Strings en donde se almacenan los datos tomados
String finalString;
String msg;

// Infrarojo
int WV;
int direccion; //Variable de direccion como entero, tomara valores (1-9)

//SIM
SoftwareSerial mySerial(5, 4); //TX,RX
String _buffer;


void setup() {
  delay(2000); 
  inicializar(); //Inicializar pines sensores efecto hall y dht
  _buffer.reserve(50);
  msg.reserve(290); //Reservar memoria en bytes para la cadena de caracteres de los datos
  mySerial.begin(9600);
  delay(1000);
  //Inicializar el modulo SIM800L en bajo consumo energetico
  mySerial.println("AT\r");
  runSerial();
  delay(1000);
  mySerial.println("AT+CSCLK=2\r");
  runSerial();
  delay(1000);
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

void inicializar() {
  //Declarar pines de la señal de los sensores efecto hall como inputs
  pinMode(hall_an, INPUT);
  pinMode(hall_pluv1, INPUT);
  pinMode(transistor_Pin, OUTPUT);
  digitalWrite(transistor_Pin, HIGH);
  //Inicializar el modulo DHT
  dht.begin();
}

void loop() {
   //Loop principal, se realizan n = "counter" toma de datos antes de hacer 
   //post en el servidor y borrar los datos del intervalo de tiempo en cuestion
  while (counter < 10){
    tomaDatos();
    digitalWrite(transistor_Pin,LOW); //Dejar de alimentar nodo con algunos sensores para ahorrar energia 
    delay(1800000);
    digitalWrite(transistor_Pin,HIGH); //Tras un tiempo dado se alimenta el nodo y se toman los datos nuevamente
  }
  delay(10000); 
  mySerial.println("AT\r");
  runSerial();
  delay(1000);
  mySerial.println("AT+CSCLK=0\r"); //Comando AT usado para establecer el modulo SIM800L en funcionamiento normal  
  runSerial();
  delay(10000);
  messageServerPost(); //Posteo del mensaje en el servidor
  msg.remove(0,290); //Se elimina el mensaje enviado
  msg.reserve(290); //Se vuelve a disponer 290bytes de memoria para el nuevo mensaje
  counter = 0;
  //Comandos AT necesarios para ordenar al SIM800L entrar en sleep mode
  mySerial.println("AT\r");
  runSerial();
  delay(1000);
  mySerial.println("AT+CSCLK=2\r");
  runSerial();
  delay(1000);
  mySerial.println("AT\r");
  runSerial();
  delay(1000);
  mySerial.println("AT+CSCLK?\r");
  runSerial();
  delay(1800000);
}

void tomaDatos() {
    hallCounter = 0;
    hallCounter1 = 0;
    hall();//Ejecutar funcion para el sensado con los efecto hall (pluviografo y anemometro)
    temp_hum();//Ejecutar funcion para el sensado de temperatura y humedad
    dir();//Ejecutar funcion para el sensado de la direccion
    time_t tiempo;
    tiempo = RTC.get(); //Obtener fecha y hora actual del RTC
    //Guardar datos en un String
    finalString = String(hour(tiempo))+ "," + String(minute(tiempo)) + "," + String(h) + "," + String(t) + "," + String(hallCounter) + "," + String(hallCounter1) + "," + String(direccion)+ ";";
    msg = msg + finalString; //Guardar la toma de diferentes ciclos en una variable que sera enviada al servidor
    Serial.println(msg);
    delay(1000);
    counter ++;
}

void hall() {
  starttime = millis();
  Serial.println(starttime);
  while ((new_endtime - starttime) < 40000) { // do this loop for up to 1000mS
    hallState = digitalRead(hall_an); //Estado hall anemometro
    hallState1 = digitalRead(hall_pluv1); //Estado hall pluviografo 1

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
  Serial.println(new_endtime);
}

 void temp_hum() {
  h = dht.readHumidity(); //Leer Humedad
  t = dht.readTemperature(); //Leer la temperatura en grados Celsius
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
   delay(2000);
   initModule(); //Inicializacion del modulo
   //Comandos AT necesarios para postear el mensaje final en el servidor
   delay(5000);
   mySerial.println(F("AT+HTTPINIT\r")); //Initializes the HTTP service. This is a proprietary Simcom AT command.This command should be sent first before starting HTTP service.
   runSerial();
   delay(500);
   mySerial.println(F("AT+HTTPPARA=\"CID\",1\r"));//sets up HTTP parameters for the HTTP call. This is a proprietary AT command from SIMCOM.
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
   mySerial.println(F("AT+HTTPACTION=1\r")); //Used perform HTTP actions such HTTP GET or HTTP post. 
   runSerial();
   delay(3000);
   mySerial.println(F("AT+HTTPREAD=0,100\r"));//Used to read the HTTP server response
   runSerial();
   delay(500);
   mySerial.println(F("AT+HTTPTERM\r")); //Terminates the HTTP session
   runSerial();
   delay(3000);
   mySerial.println(F("AT+CGATT=0\r")); //Terminates the HTTP session
   runSerial();
   delay(3000);
}

void runSerial(){
    while (mySerial.available()){
      Serial.write(mySerial.read());  
    }
}
