#include <SoftwareSerial.h>
String _buffer;

SoftwareSerial mySerial(5, 4);//TX,RX


String datos = "000";
String number = "+573012423083"; //-> change with your number


void setup() {
    delay(2000); //delay for 7 seconds to make sure the modules get the signal
    Serial.begin(9600);
    _buffer.reserve(50);
    Serial.println("Sistem Started...");
    mySerial.begin(9600);
    delay(1000);
    
    initModule();
    Serial.println("Type m to send an SMS, r to receive an SMS, and s to send message to server and g to get method");
}


void loop() {
    if (Serial.available() > 0)
      switch (Serial.read())
      {
        case 'm':
          SendMessage();
          break;
        case 's':
          messageServerPost();
          break;
        case 'g':
          messageServerGet();
          break;
      }
    if (mySerial.available() > 0)
      Serial.write(mySerial.read());
}


void initModule(){
  
    mySerial.println("AT+HTTPTERM");
    runSerial();
    delay(500);
    
    mySerial.println("AT+CPIN?");
    runSerial();
    delay(1000);
    
    mySerial.println("AT+CREG?");
    runSerial();
    delay(1000);
    
    mySerial.println("AT+CGATT?");
    runSerial();
    delay(1000);
    
    mySerial.println("AT+CSQ");
    runSerial();
    delay(1000);
    
    mySerial.println("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
    runSerial();
    delay(1500);
    
    mySerial.println("AT+SAPBR=3,1,\"APN\",\"internet.comcel.com.co\"");
    runSerial();
    delay(100);
    
    mySerial.println("AT+SAPBR=3,1,USER,\"comcel\"");
    runSerial();
    delay(100);
    
    mySerial.println("AT+SAPBR=3,1,PWD,\"comcel\"");
    runSerial();
    delay(100);
    
    mySerial.println("AT+SAPBR=0,1");
    runSerial();
    delay(1000);

    mySerial.println("AT+SAPBR=1,1");
    runSerial();
    delay(3500);
    
    mySerial.println("AT+SAPBR=2,1");
    runSerial();
    delay(2500);

}
void SendMessage()
{
  //Serial.println ("Sending Message");
    mySerial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
    runSerial();
    delay(1000);
    //Serial.println ("Set SMS Number");
    
    mySerial.println("AT+CMGS=\"" + number + "\"\r"); //Mobile phone number to send message
    runSerial();
    delay(1000);
    
    String SMS = "Hello, You got 7 days";
    mySerial.println(SMS);
    runSerial();
    delay(1000);
    
    mySerial.println((char)26);// ASCII code of CTRL+Z
    runSerial();
    delay(1000);
}

void messageServerGet()
{
     mySerial.println("AT+HTTPINIT\r");
     runSerial();
     delay(500);
     
     mySerial.println("AT+HTTPPARA=\"CID\",1\r");
     runSerial();
     delay(500);
     //mySerial.println("AT+HTTPPARA=\"URL\",\"http://eco.agromakers.org/api/v1/sensor/reporte_varios?id=2020080427&tramo=17,2,21,10,1,3,1,1,3;17,2,21,15,2,3,5,2,5;\n\r");
     mySerial.println("AT+HTTPPARA=\"URL\",\"http://eco.agromakers.org/api/v1/sensor/reporte_varios?id=2020080427&tramo=17,2,21,10,1,3,1,1,3;17,2,21,15,2,3,5,2,5;\"");
     //mySerial.println("AT+HTTPPARA=\"URL\",\"http://eco.agromakers.org/api/v1/Sensores/reporte_varios?id=2020080427&tramo="+datos+"\"");
     runSerial();
     delay(1000);
     
     mySerial.println("AT+HTTPACTION=0\r");
     runSerial();
     delay(3000);
     
     mySerial.println("AT+HTTPREAD=0,100");
     runSerial();
     delay(500);
    
     mySerial.println("AT+HTTPTERM\r");
     runSerial();
     delay(3000);
}

void messageServerPost()
{
     mySerial.println("AT+HTTPINIT\r");
     runSerial();
     delay(500);
     
     mySerial.println("AT+HTTPPARA=\"CID\",1\r");
     runSerial();
     delay(500);
     
     mySerial.println("AT+HTTPPARA=\"URL\",\"http://datos-env.iafjn3xg9q.us-east-1.elasticbeanstalk.com/datos/new/innovandes\"\r");
     runSerial();
     delay(500);
     
     mySerial.println("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
     runSerial();
     delay(500);
    
     mySerial.println("AT+HTTPDATA=1000,10000\r");
     runSerial();
     delay(1000);
    
     Serial.println("{name: \"EM1\", description: \""+ datos + "\"}");
     mySerial.println("{\"name\": \"EM1\", \"description\": \""+ datos + "\"}");
     runSerial();
     delay(10000);
     
     mySerial.println("AT+HTTPACTION=1\r");
     runSerial();
     delay(3000);
     
     mySerial.println("AT+HTTPREAD=0,100\r");
     runSerial();
     delay(500);
    
     mySerial.println("AT+HTTPTERM\r");
     runSerial();
     delay(3000);
}

void runSerial(){
    while (mySerial.available()){
      Serial.write(mySerial.read());  
    }
}
