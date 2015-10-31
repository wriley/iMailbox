/*
 * EEPROM Clear
 *
 * Sets all of the bytes of the EEPROM to 0.
 * This example code is in the public domain.
 */

#include <EEPROM.h>

void setup()
{
  Serial.begin(9600);
  delay(10);

  Serial.println("Beginning erase");
  
  EEPROM.begin(512);
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < 512; i++)
  {
    Serial.println(i);
    EEPROM.write(i, 255);
  }

  Serial.println("DONE");
  // turn the LED on when we're done
  pinMode(0, OUTPUT);
  digitalWrite(0, LOW);
  EEPROM.end();
}

void loop()
{
}
