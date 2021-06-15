#include "DHT.h" // Libreria DHT de Adafruit para DHT 11 o DHT 22 https://github.com/adafruit/DHT-sensor-library/archive/master.zip
#include <DS3232RTC.h>  //Libreria RTC https://github.com/JChristensen/DS3232RTC


#define transistor_Pin 9   //Pin del transistor

//------------------HALL----------------
#define  hall_an 3  //Pin Anemometro (Vel. Viento)
#define  hall_pluv1 8 //Pin Anemometro (Pluviografo1)

#define DHTPIN 7     //Pin donde está conectado el sensor DHT
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

int tiempo_hall = 15000;

int direccion = 0;
int WV = 0;



float h;  //Variable de humedad relativa (0-100)
float temp; //Variable temeperatura grados centigrados

boolean date_done = true;

float vol_bat = 0;
float vol_panel = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  
  dht.begin();//Inicializar el DHT

  //pinMode(A3, INPUT);
  pinMode(transistor_Pin, OUTPUT);

  digitalWrite(transistor_Pin, LOW); //Habilitar nodo 5V de alimentacion sensores

  temp_hum();
  delay(500);
  
  digitalWrite(transistor_Pin, HIGH); //Habilitar nodo 5V de alimentacion sensores
  delay(100);


  time_t tiempo = RTC.get();
  Serial.println(F("Time Datos"));
  Serial.println((String(hour(tiempo))+ "," + String(minute(tiempo))));
  initTime();
}

void loop() {
  updateVolt();
  delay(1000);
  int dirF = 0;
  temp_hum();
  delay(500);
  int contDifDir = 0;
  boolean out = true;
  while(true){
    int dir1 = dir();
    delay(1000);
    int dir2 = dir();
    delay(1000);
    int dir3 = dir();
    delay(1000);
    int dir4 = dir();
    delay(1000);
    if(dir1 == dir2){
      dirF = dir1;
      out = false;;  
    }else{
      contDifDir++;
      dirF = dir1;
    }
  }
    

  hall();
  delay(1000);
  
  time_t t = RTC.get();
  Serial.println(F("HORA"));
  Serial.println(String(hour(t))+ "," + String(minute(t)));
  
}

void initTime(){
  //Configuración de la fecha
  RTC.begin();
  if (date_done == false){
  tmElements_t tm;
  tm.Hour = 14;
  tm.Minute = 15;
  tm.Second = 00;
  tm.Day = 14;
  tm.Month = 06;
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
