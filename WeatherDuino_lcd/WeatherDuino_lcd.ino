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
//       mess up the JSON output from the WeatherDuino
DHT dht1(3, DHT11); // #1 is connected to Pin3 and type is DHT11
DHT dht2(4, DHT11); // #2 is connected to Pin4 and type is DHT11
DHT dht3(5, DHT11); // #2 is connected to Pin4 and type is DHT11
DHT dht4(6, DHT11); // #2 is connected to Pin4 and type is DHT11

  
// LCD Panel Connections:
// rs (LCD pin 4) to Arduino pin 13
// rw (LCD pin 5) to Arduino pin 12
// enable (LCD pin 6) to Arduino pin 11
// LCD pins d4, d5, d6, d7 to Arduino pins 10, 9, 8, 7
LiquidCrystal lcd(13, 12, 11, 10, 9, 8, 7);

// Data wire is plugged into port 2 on the Arduino
// Setup a oneWire instance to communicate with any OneWire devices (not just 
// Maxim/Dallas temperature ICs)
OneWire oneWire(2);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature onewire(&oneWire);
DeviceAddress Probe01 = { 0x28, 0x0B, 0x4D, 0xC6, 0x04, 0x00, 0x00, 0x4D }; 
DeviceAddress Probe02 = { 0x28, 0x35, 0xD8, 0xC6, 0x04, 0x00, 0x00, 0xD6 };
DeviceAddress Probe03 = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
DeviceAddress Probe04 = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Set the basic array, by default assume all probes present.
// Temperature probes are index 0-3, humidity are index 4-7.
bool probes[8];
float hum;
float temp;

void setup() {
  Serial.begin(57600); 
  lcd.begin(16,4);              // columns, rows.  use 16,2 for a 16x2 LCD, etc.
  lcd.clear();                  // start with a blank screen
  lcd.setCursor(0,0);           // set cursor to column 0, row 0 (the first row)
  lcd.print("WeatherDuino!");
  lcd.setCursor(0,1);
  lcd.print("v2.00");
  lcd.setCursor(-4,2);          // Line 3 and Line 4 are off by 4 chars.
  lcd.print("by Fludizz");
  lcd.setCursor(-4,3);
  lcd.print("DHT11 & DS18B20");
  delay(1000);                  // Show bootscreen for 1 second!

  // Start the probes
  dht1.begin();
  dht2.begin();
  dht3.begin();
  dht4.begin();
  onewire.begin();
  
  bool probes[8] = {true, true, true, true, true, true, true, true};
  // Return a Boolean array of present probes.
  // Request a temperature measurement from the OneWire Probes
  onewire.requestTemperatures();
  // Check each onewire probe for valid response. Invalid response is 85.
  if ( onewire.getTempC(Probe01) == 85 ) {
    probes[0] = false;
  }
  if ( onewire.getTempC(Probe02) == 85 ) {
    probes[1] = false;
  }
  if ( onewire.getTempC(Probe03) == 85 ) {
    probes[2] = false;
  }
  if ( onewire.getTempC(Probe04) == 85 ) {
    probes[3] = false;
  }
  // The DHT probes have two return options: -127 or NaN, to avoid extra polling
  // stick the return from the Read in a Variable and reuse this for each.
  hum = dht1.readHumidity();
  if ( hum == -127 || isnan(hum) ) {
    probes[4] = false;
  }
  hum = dht2.readHumidity();
  if ( hum == -127 || isnan(hum) ) {
    probes[5] = false;
  }
  hum = dht3.readHumidity();
  if ( hum == -127 || isnan(hum) ) {
    probes[6] = false;
  }
  hum = dht4.readHumidity();
  if ( hum == -127 || isnan(hum) ) {
    probes[7] = false;
  }
  
  // Start preparing the initial screen!
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("P1:");
  lcd.setCursor(0,1);
  lcd.print("P2:");
  lcd.setCursor(-4,2);
  lcd.print("P3:");
  lcd.setCursor(-4,3);
  lcd.print("P4:");
}

void loop() {
  // Request a temperature measurement from the OneWire Probes
  onewire.requestTemperatures();
  // Set all readings to the "No Sensor value"
  float readings[8] = { -127, -127, -127, -127, -127, -127, -127, -127 };
  // Read all present (True) ds18b20 probes
  if ( probes[0] ) {
    temp = onewire.getTempC(Probe01);
    if ( temp != 85 ) {
      readings[0] = temp;
    }
  }
  if ( probes[1] ) {
    temp = onewire.getTempC(Probe02);
    if ( temp != 85 ) {
      readings[1] = temp;
    }
  }
  if ( probes[2] ) {
    temp = onewire.getTempC(Probe03);
    if ( temp != 85 ) {
      readings[2] = temp;
    }
  }
  if ( probes[3] ) {
    temp = onewire.getTempC(Probe04);
    if ( temp != 85 ) {
      readings[3] = temp;
    }
  }
  // Read all present (True) dht11 probes
  if ( probes[4] ) {
    hum = dht1.readHumidity();
    if ( isnan(hum) == 0 ) {
      readings[4] = hum;
    }
  }
  if ( probes[5] ) {
    hum = dht2.readHumidity();
    if ( isnan(hum) == 0 ) {
      readings[5] = hum;
    }
  }
  if ( probes[6] ) {
    hum = dht3.readHumidity();
    if ( isnan(hum) == 0 ) {
      readings[6] = hum;
    }
  }
  if ( probes[7] ) {
    hum = dht4.readHumidity();
    if ( isnan(hum) == 0 ) {
      readings[7] = hum;
    }
  }

  // Dump the sensor values as JSON on Serial
  Serial.print("{\"WeatherDuino\":[{\"probe\":1,\"temp\":"); 
  Serial.print(readings[0]);
  Serial.print(",\"humid\":");
  Serial.print(readings[4]);
  Serial.print("},{\"probe\":2,\"temp\":");
  Serial.print(readings[1]);
  Serial.print(",\"humid\":");
  Serial.print(readings[5]);
  Serial.print("},{\"probe\":3,\"temp\":");
  Serial.print(readings[2]);
  Serial.print(",\"humid\":");
  Serial.print(readings[6]);
  Serial.print("},{\"probe\":4,\"temp\":");
  Serial.print(readings[3]);
  Serial.print(",\"humid\":");
  Serial.print(readings[7]);
  Serial.println("}]}");

  // LCD Line 1:
  lcd.setCursor(4,0);
  if ( readings[0] == -127 ) {
    lcd.print("--.-");
  } else if ( readings[0] < 10 && readings[0] > 0 ) {
    lcd.print(" ");
    lcd.print(readings[0], 1);
  } else {
    lcd.print(readings[0], 1);
  }
  lcd.print(char(223));
  lcd.print("C ");
  if ( readings[4] == -127 ) {
    lcd.print("--");
  } else if ( readings[4] < 10 ) {
    lcd.print(" ");
    lcd.print(readings[4], 0);
  } else {
    lcd.print(readings[4], 0);
  }
  lcd.print("%");

  // LCD line 2
  lcd.setCursor(4,1);
  if ( readings[1] == -127 ) {
    lcd.print("--.-");
  } else if ( readings[1] < 10 && readings[1] > 0 ) {
    lcd.print(" ");
    lcd.print(readings[1], 1);
  } else {
    lcd.print(readings[1], 1);
  }
  lcd.print(char(223));
  lcd.print("C ");
  if ( readings[5] == -127 ) {
    lcd.print("--");
  } else if ( readings[5] < 10 ) {
    lcd.print(" ");
    lcd.print(readings[5], 0);
  } else {
    lcd.print(readings[5], 0);
  }
  lcd.print("%");

  // LCD line 3 - this line starts at "-4".  
  lcd.setCursor(0,2);
  if ( readings[2] == -127 ) {
    lcd.print("--.-");
  } else if ( readings[2] < 10 && readings[2] > 0 ) {
    lcd.print(" ");
    lcd.print(readings[2], 1);
  } else {
    lcd.print(readings[2], 1);
  }
  lcd.print(char(223));
  lcd.print("C ");
  if ( readings[6] == -127 ) {
    lcd.print("--");
  } else if ( readings[6] < 10 ) {
    lcd.print(" ");
    lcd.print(readings[6], 0);
  } else {
    lcd.print(readings[6], 0);
  }
  lcd.print("%");

  // LCD line 4 - this line starts at "-4".  
  lcd.setCursor(0,3);
  if ( readings[3] == -127 ) {
    lcd.print("--.-");
  } else if ( readings[3] < 10 && readings[3] > 0 ) {
    lcd.print(" ");
    lcd.print(readings[3], 1);
  } else {
    lcd.print(readings[3], 1);
  }
  lcd.print(char(223));
  lcd.print("C ");
  if ( readings[7] == -127 ) {
    lcd.print("--");
  } else if ( readings[7] < 10 ) {
    lcd.print(" ");
    lcd.print(readings[7], 0);
  } else {
    lcd.print(readings[7], 0);
  }
  lcd.print("%");
}

