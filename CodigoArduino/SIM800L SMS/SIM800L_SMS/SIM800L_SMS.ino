#include <SoftwareSerial.h>
SoftwareSerial sim(3, 2);//TX,RX
int _timeout;
String _buffer;
String number = "+573225690894"; //-> change with your number

String apn = "internet.comcel.com.co";
String apn_usr = "comcel";
String apn_pwd = "comcel";


String urlPost = "http://datos-env.iafjn3xg9q.us-east-1.elasticbeanstalk.com/datos/new/innovandes";
String dato1 = "NamePost";
String dato2 = "Desc Post";


void setup() {
  delay(4000); //delay for 7 seconds to make sure the modules get the signal
  Serial.begin(9600);
  _buffer.reserve(50);
  Serial.println("Sistem Started...");
  sim.begin(9600);
  delay(1000);
  Serial.println("Initializing...");
  delay(1000);

  mySerial.println("AT"); //Once the handshake test is successful, it will back to OK
  updateSerial();
  mySerial.println("AT+CSQ"); //Signal quality test, value range is 0-31 , 31 is the best
  updateSerial();
  mySerial.println("AT+CCID"); //Read SIM information to confirm whether the SIM is plugged
  updateSerial();
  mySerial.println("AT+CREG?"); //Check whether it has registered in the network
  updateSerial();
  Serial.println("Type m to send an SMS, r to receive an SMS, and s to send message to server and g to get method");
}

void SendMessage()
{
  //Serial.println ("Sending Message");
  sim.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  delay(1000);
  //Serial.println ("Set SMS Number");
  sim.println("AT+CMGS=\"" + number + "\"\r"); //Mobile phone number to send message
  delay(1000);
  String SMS = "Hello, You got 7 days";
  sim.println(SMS);
  delay(100);
  sim.println((char)26);// ASCII code of CTRL+Z
  delay(1000);
  _buffer = _readSerial();
}
void RecieveMessage()
{
  Serial.println ("SIM800L Read an SMS");
  delay (1000);
  sim.println("AT+CNMI=2,2,0,0,0"); // AT Command to receive a live SMS
  delay(1000);
  Serial.write ("Unread Message done");
}
String _readSerial() {
  _timeout = 0;
  while  (!sim.available() && _timeout < 12000  )
  {
    delay(13);
    _timeout++;
  }
  if (sim.available()) {
    return sim.readString();
  }
}
void callNumber() {
  sim.print (F("ATD"));
  sim.print (number);
  sim.print (F(";\r\n"));
  _buffer = _readSerial();
  Serial.println(_buffer);
}
void messageServerGet()
{
   sim.println("AT+SAPBR=3,1,Contype,GPRS");
   delay(100);
   sim.println("AT+SAPBR=3,1,\"APN\",\"internet.comcel.com.co\"\r\n");
   delay(100);
   sim.println("AT+SAPBR=3,1,\"USER\",\"comcel\"\r\n");
   delay(100);
   sim.println("AT+SAPBR=3,1,\"PWD\",\"comcel\"\r\n");
   delay(1000);
   sim.println("AT+SAPBR=1,1");
   delay(1000);
   sim.println("AT+HTTPINIT");
   delay(100);
   sim.println("AT+HTTPPARA=\"CID\",1");
   delay(100);
   sim.println("AT+HTTPPARA=\"URL\",\"\"\r\n");
   delay(100);
   sim.println("AT+HTTPACTION=0");
   delay(1000);
   sim.println("AT+HTTPREAD=0,100");
}
void messageServerPost()
{
 delay(100);
 sim.println("AT+SAPBR=3,1,Contype,GPRS");
 delay(100);
 sim.println("AT+SAPBR=3,1,APN," + apn);
 runSerial();
 delay(100);
 sim.println("AT+SAPBR=3,1,USER," + apn_usr);
 runSerial();
 delay(100);
 sim.println("AT+SAPBR=3,1,PWD," + apn_pwd);
 runSerial();
 delay(100);
 sim.println("AT+SAPBR=1,1");
 runSerial();
 delay(100);
 sim.println("AT+HTTPINIT");
 runSerial();
 delay(100);
 sim.println("AT+HTTPPARA=CID,1");
 runSerial();
 delay(100);
 sim.println("AT+HTTPPARA=\"URL\",\""+ urlPost +"\"\r\n");
 runSerial();
 delay(100);
 sim.println("AT+HTTPPARA=CONTENT,application/json");
 runSerial();
 delay(100);
 sim.println("AT+HTTPDATA=192,10000");
 runSerial();
 delay(100);
 sim.println("{ }");
 runSerial();
 delay(10000);
 sim.println("AT+HTTPACTION=1");
 runSerial();
 delay(5000);
 sim.println("AT+HTTPREAD");
 runSerial();
 delay(100);
 sim.println("AT+HTTPTERM");

}
void runSerial(){
  while (sim.available()){
    Serial.write(sim.read());  
  }
}
void loop() {
  if (Serial.available() > 0)
    switch (Serial.read())
    {
      case 'm':
        SendMessage();
        break;
      case 'r':
        RecieveMessage();
        break;
      case 's':
        messageServerPost();
        break;
    }
  if (sim.available() > 0)
    Serial.write(sim.read());
}
