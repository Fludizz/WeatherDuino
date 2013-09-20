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
DHT dht0(3, DHT11); // #1 is connected to Pin3 and type is dht11
DHT dht1(4, DHT11); // #2 is connected to Pin4 and type is dht11
DHT dht2(5, DHT11); // #3 is connected to Pin5 and type is dht11
DHT dht3(6, DHT11); // #4 is connected to Pin6 and type is dht11

  
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
DeviceAddress Probe0 = { 0x28, 0x0B, 0x4D, 0xC6, 0x04, 0x00, 0x00, 0x4D }; 
DeviceAddress Probe1 = { 0x28, 0x35, 0xD8, 0xC6, 0x04, 0x00, 0x00, 0xD6 };
DeviceAddress Probe2 = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
DeviceAddress Probe3 = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Configure which probes are attached.
const bool probes[] = {true, true, false, false };

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
  dht0.begin();
  dht1.begin();
  dht2.begin();
  dht3.begin();
  onewire.begin();
  
  // Start preparing the initial screen!
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("P0:");
  lcd.setCursor(0,1);
  lcd.print("P1:");
  lcd.setCursor(-4,2);
  lcd.print("P2:");
  lcd.setCursor(-4,3);
  lcd.print("P3:");
}

void loop() {
  // Request a temperature measurement from the OneWire Probes
  onewire.requestTemperatures();
  // Set all readings to the "No Sensor value"
  float temps[] = { -127, -127, -127, -127 };
  float hums[] = { -127, -127, -127, -127 };
  float hum;
  float temp;
  // Read the DS18B20 and DHT11 for each configured probe.
  if ( probes[0] ) {
    temp = onewire.getTempC(Probe0);
    if ( temp != 85 ) {
      temps[0] = temp;
    }
    hum = dht0.readHumidity();
    if ( isnan(hum) != 1 || hum != 0 ) {
      hums[0] = hum;
    }
  }
  if ( probes[1] ) {
    temp = onewire.getTempC(Probe1);
    if ( temp != 85 ) {
      temps[1] = temp;
    }
    hum = dht1.readHumidity();
    if ( isnan(hum) != 1 || hum != 0 ) {
      hums[1] = hum;
    }
  }
  if ( probes[2] ) {
    Serial.println("Probe2 true");
    temp = onewire.getTempC(Probe2);
    if ( temp != 85 ) {
      temps[2] = temp;
    }
    hum = dht2.readHumidity();
    if ( isnan(hum) != 1 || hum != 0 ) {
      hums[2] = hum;
    }
  }
  if ( probes[3] ) {
    temp = onewire.getTempC(Probe3);
    if ( temp != 85 ) {
      temps[3] = temp;
    }
    hum = dht3.readHumidity();
    if ( isnan(hum) != 1 || hum != 0 ) {
      hums[3] = hum;
    }
  }

  // Dump the sensor values as JSON on Serial
  Serial.print("{\"WeatherDuino\":[{\"probe\":0,\"temp\":"); 
  Serial.print(temps[0]);
  Serial.print(",\"humid\":");
  Serial.print(hums[0]);
  Serial.print("},{\"probe\":1,\"temp\":");
  Serial.print(temps[1]);
  Serial.print(",\"humid\":");
  Serial.print(hums[1]);
  Serial.print("},{\"probe\":2,\"temp\":");
  Serial.print(temps[2]);
  Serial.print(",\"humid\":");
  Serial.print(hums[2]);
  Serial.print("},{\"probe\":3,\"temp\":");
  Serial.print(temps[3]);
  Serial.print(",\"humid\":");
  Serial.print(hums[3]);
  Serial.println("}]}");

  // LCD Line 1:
  lcd.setCursor(4,0);
  if ( temps[0] == -127 ) {
    lcd.print("--.-");
  } else if ( temps[0] < 10 && temps[0] > 0 ) {
    lcd.print(" ");
    lcd.print(temps[0], 1);
  } else {
    lcd.print(temps[0], 1);
  }
  lcd.print(char(223));
  lcd.print("C ");
  if ( hums[0] == -127 ) {
    lcd.print("--");
  } else if ( hums[0] < 10 ) {
    lcd.print(" ");
    lcd.print(hums[0], 0);
  } else {
    lcd.print(hums[0], 0);
  }
  lcd.print("%");

  // LCD line 2
  lcd.setCursor(4,1);
  if ( temps[1] == -127 ) {
    lcd.print("--.-");
  } else if ( temps[1] < 10 && temps[1] > 0 ) {
    lcd.print(" ");
    lcd.print(temps[1], 1);
  } else {
    lcd.print(temps[1], 1);
  }
  lcd.print(char(223));
  lcd.print("C ");
  if ( hums[1] == -127 ) {
    lcd.print("--");
  } else if ( hums[1] < 10 ) {
    lcd.print(" ");
    lcd.print(hums[1], 0);
  } else {
    lcd.print(hums[1], 0);
  }
  lcd.print("%");

  // LCD line 3 - this line starts at "-4".  
  lcd.setCursor(0,2);
  if ( temps[2] == -127 ) {
    lcd.print("--.-");
  } else if ( temps[2] < 10 && temps[2] > 0 ) {
    lcd.print(" ");
    lcd.print(temps[2], 1);
  } else {
    lcd.print(temps[2], 1);
  }
  lcd.print(char(223));
  lcd.print("C ");
  if ( hums[2] == -127 ) {
    lcd.print("--");
  } else if ( hums[2] < 10 ) {
    lcd.print(" ");
    lcd.print(hums[2], 0);
  } else {
    lcd.print(hums[2], 0);
  }
  lcd.print("%");

  // LCD line 4 - this line starts at "-4".  
  lcd.setCursor(0,3);
  if ( temps[3] == -127 ) {
    lcd.print("--.-");
  } else if ( temps[3] < 10 && temps[3] > 0 ) {
    lcd.print(" ");
    lcd.print(temps[3], 1);
  } else {
    lcd.print(temps[3], 1);
  }
  lcd.print(char(223));
  lcd.print("C ");
  if ( hums[3] == -127 ) {
    lcd.print("--");
  } else if ( hums[3] < 10 ) {
    lcd.print(" ");
    lcd.print(hums[3], 0);
  } else {
    lcd.print(hums[3], 0);
  }
  lcd.print("%");
}

