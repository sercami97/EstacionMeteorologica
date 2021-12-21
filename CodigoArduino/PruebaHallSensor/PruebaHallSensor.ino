/*
 KeyPressed on PIN1
 by Mischianti Renzo <http://www.mischianti.org>

 https://www.mischianti.org/2019/01/02/pcf8574-i2c-digital-i-o-expander-fast-easy-usage/
*/

#include "Arduino.h"
#include "PCF8574.h"

// Set i2c address
PCF8574 pcf8574(0x20);
unsigned long timeElapsed;
void setup()
{
	Serial.begin(115200);
	delay(1000);
  Serial.println("Begin");

  pcf8574.pinMode(P0, INPUT_PULLUP);
  pcf8574.pinMode(P1, INPUT_PULLUP);
  pcf8574.pinMode(P2, INPUT_PULLUP);
  pcf8574.pinMode(P3, INPUT_PULLUP);
  pcf8574.pinMode(P4, INPUT_PULLUP);
  pcf8574.pinMode(P5, INPUT_PULLUP);
  pcf8574.pinMode(P6, INPUT_PULLUP);
  pcf8574.pinMode(P7, INPUT_PULLUP);

	Serial.print("Init pcf8574...");
	if (pcf8574.begin()){
		Serial.println("OK");
	}else{
		Serial.println("KO");
	}
}

void loop()
{

  if(!pcf8574.digitalRead(P0)) Serial.println("KEY 1 PRESSED");
  if(!pcf8574.digitalRead(P1)) Serial.println("KEY 2 PRESSED");
  if(!pcf8574.digitalRead(P2)) Serial.println("KEY 3 PRESSED");
  if(!pcf8574.digitalRead(P3)) Serial.println("KEY 4 PRESSED");
  if(!pcf8574.digitalRead(P4)) Serial.println("KEY 5 PRESSED");
  if(!pcf8574.digitalRead(P5)) Serial.println("KEY 6 PRESSED");
  if(!pcf8574.digitalRead(P6)) Serial.println("KEY 7 PRESSED");
  if(!pcf8574.digitalRead(P7)) Serial.println("KEY 8 PRESSED");
  
}
