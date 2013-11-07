#include <DHT.h>
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// DHT sensor settings
// Connect DHT pin 1 (on the left) of the sensor to +5V
// Connect DHT pin 2 of the sensor to whatever Datapin you choose
// Connect DHT pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
// Note: Edit the DHT.cpp file, remove the Serial.print() lines. Those prints
//       mess up the Serial output from the WeatherDuino
DHT dht[4] = 
{
  DHT(3, DHT11), // #1 is connected to Pin3 and type is dht11
  DHT(4, DHT11), // #2 is connected to Pin4 and type is dht11
  DHT(5, DHT11), // #3 is connected to Pin5 and type is dht11
  DHT(6, DHT11)  // #4 is connected to Pin6 and type is dht11
};

// OneWire DS18B20 Sensors settings
// Data wire is plugged into port 2 on the Arduino
// Setup a oneWire instance to communicate with any OneWire devices (not just 
// Maxim/Dallas temperature ICs)
OneWire oneWire(2);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature onewire(&oneWire);
// Add all four probes to an array for easy for-loop processing!
DeviceAddress Probe[4] = 
{
  { 0x28, 0x0B, 0x4D, 0xC6, 0x04, 0x00, 0x00, 0x4D }, // OneWire probe #1
  { 0x28, 0x35, 0xD8, 0xC6, 0x04, 0x00, 0x00, 0xD6 }, // OneWire probe #2
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // OneWire probe #3
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }  // OneWire probe #4
};


// LCD Panel Connections:
// rs (LCD pin 4) to Arduino pin 13
// rw (LCD pin 5) to Arduino pin 12
// enable (LCD pin 6) to Arduino pin 11
// LCD pins d4, d5, d6, d7 to Arduino pins 10, 9, 8, 7
LiquidCrystal lcd(13, 12, 11, 10, 9, 8, 7);


void setup() {
  Serial.begin(57600); 
  lcd.begin(16,4);              // columns, rows.  use 16,2 for a 16x2 LCD, etc.
  lcd.clear();                  // start with a blank screen
  lcd.setCursor(0,0);           // set cursor to column 0, row 0 (the first row)
  lcd.print("WeatherDuino!");
  lcd.setCursor(0,1);
  lcd.print("v2.00");
  lcd.setCursor(0,2);          // Line 3 and Line 4 are off by -4 chars on 16x4 displays!
  lcd.print("by Fludizz");
  lcd.setCursor(0,3);
  lcd.print("DHT11 & DS18B20");
  delay(1000);                  // Show bootscreen for 1 second!

  // Start the probes
  dht[0].begin();
  dht[1].begin();
  dht[2].begin();
  dht[3].begin();
  onewire.begin();
  
  // Start preparing the initial screen!
  lcd.clear();
  for ( int i = 0; i < 4; i++ ) {
    lcd.setCursor(0,i);
    lcd.print("Probe ");
    lcd.print(int(i + 1));
    lcd.print(":");
    lcd.setCursor(14,i);
    lcd.print(char(223));
    lcd.print("C");
    lcd.setCursor(19,i);
    lcd.print("%");
  }
}

void loop() {
  // Request a temperature measurement from the OneWire Probes
  onewire.requestTemperatures();
  // Set all readings to the "No Sensor value"
  float temps[] = { -127, -127, -127, -127 };
  float hums[] = { -127, -127, -127, -127 };
  float hum;
  float temp;
  // Read all the DS18B20 and DHT11 sensors
  for (int i = 0; i < 4; i++) {
    temp = onewire.getTempC(Probe[i]);
    if ( temp != 85 ) {
      temps[i] = temp;
    }
    hum = dht[i].readHumidity();
    if ( isnan(hum) != 1 && hum != 0 ) {
      hums[i] = hum;
    }
  }
  
  /* Test value generation
  temps[0] = random(-20, 75);
  temps[1] = random(-20, 75);
  temps[2] = random(-20, 75);
  temps[3] = random(-20, 75);
  hums[0] = random(5, 95);
  hums[1] = random(5, 95);
  hums[2] = random(5, 95);
  hums[3] = random(5, 95);
  delay(1000);
  /* Uncomment above section for testing! */

  // Dump the sensor values as JSON on Serial
  Serial.print("{\"WeatherDuino\":[");
  for (int i = 0; i < 4; i++) {
    Serial.print("{\"probe\":");
    Serial.print(int(i + 1));
    Serial.print(",\"temp\":");
    Serial.print(temps[i]);
    Serial.print(",\"humid\":");
    Serial.print(hums[i]);
    // Max 4 probes, should not place the comma after the last probe
    if ( i < 3 ) {
      Serial.print("},");
    }
  }
  // To reduce Serial.Print() lines in the for loop, add the last
  // closing } for the last probe down here as well.
  Serial.println("}]}");
  
  // Write Temperature and humidity sensor values to LCD.
  for ( int i = 0; i < 4; i++ ) {
    lcd.setCursor(9,i);
    if ( temps[i] == -127 ) {
      lcd.print(" --.-");
    } else if ( temps[i] < 10 && temps[i] >= 0 ) {
      lcd.print("  ");
      lcd.print(temps[i], 1);
    } else if ( temps[i] < -9 ) {
      lcd.print(temps[i], 1);
    } else {
      lcd.print(" ");
      lcd.print(temps[i], 1);
    }
    
    lcd.setCursor(17,i);
    if ( hums[i] == -127 ) {
      lcd.print("--");
    } else if ( hums[i] < 10 ) {
      lcd.print(" ");
      lcd.print(hums[i], 0);
    } else {
      lcd.print(hums[i], 0);
    }
  }
}
