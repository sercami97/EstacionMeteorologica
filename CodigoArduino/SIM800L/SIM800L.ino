#include <SoftwareSerial.h>
SoftwareSerial sim(5,4);//TX,RX
String _buffer;

String datos = "Datoooooos";


void setup() {
    delay(2000); //delay for 7 seconds to make sure the modules get the signal
    Serial.begin(9600);
    _buffer.reserve(50);
    Serial.println("Sistem Started...");
    sim.begin(9600);
    delay(1000);
    
    initModule();
}


void loop() {
    if (Serial.available() > 0)
      switch (Serial.read())
      {
        case 's':
          messageServerPost();
          break;
      }
    if (sim.available() > 0)
      Serial.write(sim.read());
}


void initModule(){
    
    sim.println("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
    runSerial();
    delay(1500);
    
    sim.println("AT+SAPBR=3,1,\"APN\",\"internet.comcel.com.co\"");
    runSerial();
    delay(100);
    
    sim.println("AT+SAPBR=3,1,USER,\"comcel\"");
    runSerial();
    delay(100);
    
    sim.println("AT+SAPBR=3,1,PWD,\"comcel\"");
    runSerial();
    delay(100);
    
    sim.println("AT+SAPBR=1,1");
    runSerial();
    delay(3500);
    
    sim.println("AT+SAPBR=2,1");
    runSerial();
    delay(2500);

}

void messageServerPost()
{
     sim.println("AT+HTTPINIT\r");
     runSerial();
     delay(500);
     
     sim.println("AT+HTTPPARA=\"CID\",1\r");
     runSerial();
     delay(500);
     
     sim.println("AT+HTTPPARA=\"URL\",\"http://datos-env.iafjn3xg9q.us-east-1.elasticbeanstalk.com/datos/new/innovandes\"\r");
     runSerial();
     delay(500);
     
     sim.println("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
     runSerial();
     delay(500);
    
     sim.println("AT+HTTPDATA=1000,10000\r");
     runSerial();
     delay(1000);

     sim.println("{\"name\": \"EM1\", \"description\": \""+ datos + "\"}");
     runSerial();
     delay(10000);
     
     sim.println("AT+HTTPACTION=1\r");
     runSerial();
     delay(3000);
     
     sim.println("AT+HTTPREAD=0,100\r");
     runSerial();
     delay(500);
    
     sim.println("AT+HTTPTERM\r");
     runSerial();
     delay(3000);
}

void runSerial(){
    while (sim.available()){
      Serial.write(sim.read());  
    }
}
