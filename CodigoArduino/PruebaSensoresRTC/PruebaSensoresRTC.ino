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

int lastHall1 = 0;    //Estado previo del sensor efecto hall pluviografo (HIGH,LOW)
int lLastHall1 = 0;     //Estado previo del sensor efecto hall anemometro (HIGH,LOW)
int lLLastHall1 = 0;     //Estado previo del sensor efecto hall anemometro (HIGH,LOW)

float starttime;
float new_endtime = 0;

int tiempo_hall = 30000;

int direccion = 0;
int WV = 0;



float h;  //Variable de humedad relativa (0-100)
float temp; //Variable temeperatura grados centigrados

boolean date_done = false;

float vol_bat = 0;
float vol_panel = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  dht.begin();//Inicializar el DHT

  digitalWrite(transistor_Pin, HIGH); //Habilitar nodo 5V de alimentacion sensores
  delay(100);

  date_done = true;
  //initTime();
}

void loop() {
  updateVolt();
  delay(1000);
  int dirF = 0;
  temp_hum();
  delay(500);
  int contDifDir = 0;
  boolean out = true;
  while(out and contDifDir<3){
    int dir1 = dir();
    delay(1000);
    int dir2 = dir();
    delay(1000);
    if(dir1 == dir2){
      dirF = dir1;
      out = false;;  
    }else{
      contDifDir++;
      dirF = dir1;
    }
  }
    

  //hall();
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
  tm.Hour = 20;
  tm.Minute = 19;
  tm.Second = 00;
  tm.Day = 24;
  tm.Month = 03;
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
        Serial.println(hallCounter);
        delay(500);
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

    if (WV > 145 && WV < 169){
      direccion = 1;
    }
    else if (WV >= 169 && WV < 175){
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
