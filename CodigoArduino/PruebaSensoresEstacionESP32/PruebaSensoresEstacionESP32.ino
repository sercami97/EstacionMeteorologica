#include "DHT.h" // Libreria DHT de Adafruit para DHT 11 o DHT 22 https://github.com/adafruit/DHT-sensor-library/archive/master.zip


#define transistor_Pin 12   //Pin del transistor
#define transistor_sim 32   //Pin del transistor
#define transistor_aux 33   //Pin del transistor

#define LED_GPIOR   5   //
#define LED_GPIOG   18  //
#define LED_GPIOB   19  //

#define PWM1_ChR    0
#define PWM1_ChG    1
#define PWM1_ChB    2

#define PWM1_Res   4
#define PWM1_Freq  1000

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


const uint8_t LIMIT_CONT = 35;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println(F("BEGIN"));

  
  pinMode(transistor_Pin, OUTPUT);
  pinMode(transistor_sim, OUTPUT);
  pinMode(transistor_aux, OUTPUT);

  initRGB();
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


//  sendGet(URL);
  
  dht.begin();//Inicializar el DHT
  delay(500);

  temp_hum();
  delay(500);

  delay(1000);
 
  
}

void loop() {
  
  Serial.println("LOOP");
  testRGB();
  delay(1000);

  
}

 
int PWM1_DutyCycle = 0;

void initRGB(){
  
  ledcAttachPin(LED_GPIOR, PWM1_ChR);
  ledcSetup(PWM1_ChR, PWM1_Freq, PWM1_Res);

  ledcAttachPin(LED_GPIOG, PWM1_ChG);
  ledcSetup(PWM1_ChG, PWM1_Freq, PWM1_Res);

  ledcAttachPin(LED_GPIOB, PWM1_ChB);
  ledcSetup(PWM1_ChB, PWM1_Freq, PWM1_Res);
  
}
void setColorRGB(uint8_t R, uint8_t G, uint8_t B){
  
  ledcWrite(PWM1_ChR, R);
  delay(100);
  ledcWrite(PWM1_ChG, G);
  delay(100);
  ledcWrite(PWM1_ChB, B);
  delay(100);
    
}
void testRGB(){

  delay(2000);
  Serial.println("BLU");
  setColorRGB(255,255,0);
  delay(3000);
  Serial.println("GREEN");
  setColorRGB(255,0,255);
  delay(2000);
  Serial.println("RED");
  setColorRGB(0,255,255);
}

void testSensores(){
  updateVolt();
  delay(1000);
  int dirF = 0;
  temp_hum();
  delay(500);
  int contDifDir = 0;
  boolean out = true;  
  delay(100000);
}


void temp_hum() {
  h = dht.readHumidity(); //Leer la humedad relativa
  temp = dht.readTemperature(); //Leer temperatura en grados Celsius
  Serial.println("Datos Ambiente");
  Serial.println(String(h) + " - " + String(temp));
}

void updateVolt(){
//  vol_bat = float (analogRead(A1)*(0.0049)*2);
//  vol_panel = float (analogRead(A2)*(4.73)*(0.0049));
//  Serial.println(String(vol_bat) + " - " + String(vol_panel));  
    Serial.println("Voltage");
}
