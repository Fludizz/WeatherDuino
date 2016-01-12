#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <DallasTemperature.h>
#include <DHT.h>
// lets include https://github.com/markruys/arduino-DHT/blob/master/DHT.cpp
#include <OneWire.h>

// DHT sensor settings
// Connect DHT pin 1 (on the left) of the sensor to +5V
// Connect DHT pin 2 of the sensor to whatever Datapin you choose
// Connect DHT pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
// Note: Edit the DHT.cpp file, remove the Serial.print() lines. Those prints
//       mess up the Serial output from the WeatherDuino
byte dhtpins[4] = { 3, 4, 5, 6 };
DHT dht[4] = {};

// OneWire DS18B20 Sensors settings
// Data wire is plugged into port 2 on the Arduino
// Setup a oneWire instance to communicate with any OneWire devices (not just 
// Maxim/Dallas temperature ICs)
static byte oneWirepin = 2;
OneWire oneWire(oneWirepin);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature dallas(&oneWire);
DeviceAddress Probes[4] = {//initialize to zero
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};


// LCD Panel Connections:
// rs (LCD pin 4) to Arduino pin 13
// rw (LCD pin 5) to Arduino pin 12
// enable (LCD pin 6) to Arduino pin 11
// LCD pins d4, d5, d6, d7 to Arduino pins 10, 9, 8, 7
LiquidCrystal lcd(13, 12, 11, 10, 9, 8, 7);

// store onewire addresses from the 16th bit in eeprom
const byte eeprom_sensorcountoffset = 12; // to 14
const byte eeprom_addressoffset = 16; // to 16+(4*8)

int eeprom_numberofsensors;

void setup() {
  Serial.begin(57600); 

  lcd.begin(20,4);              // columns, rows.  use 16,2 for a 16x2 LCD, etc.
  lcd.clear();                  // start with a blank screen
  lcd.setCursor(0,0);           // set cursor to column 0, row 0 (the first row)
  lcd.print("WeatherDuino!");
  lcd.setCursor(0,1);
  lcd.print("v2.01");
  lcd.setCursor(0,2);          // Line 3 and Line 4 are off by -4 chars on 16x4 displays!
  lcd.print("by Fludizz");
  lcd.setCursor(0,3);
  lcd.print("DHTxx & DS18x20");
  delay(500);                  // Show bootscreen for 0.5 seconds!

  eeprom_numberofsensors = EEPROM.read(eeprom_sensorcountoffset);
  lcd.setCursor(0,0);
  lcd.print("Sensors stored: ");
  lcd.print(eeprom_numberofsensors);
  Serial.print("Sensors stored in EEPROM: ");
  Serial.println(eeprom_numberofsensors);

  dallas.begin();
  if (eeprom_numberofsensors > 4 ||
      eeprom_numberofsensors == 0 ||
      eeprom_numberofsensors != dallas.getDeviceCount()){
    // we should trigger a scan, the number of probes has changed!
    EEPROM.write(eeprom_sensorcountoffset, 0);
    eeprom_numberofsensors = 0;
    lcd.setCursor(0,1);
    lcd.print("Initializing...");
    lcd.setCursor(0,2);
    lcd.print("Connect sensors");
    find_onewire_devices();
  } else {
    read_onewire_addresses();
  }
  lcd.setCursor(0,4);
  lcd.print("Ready.");
  Serial.println("Sensors read from EEPROM, ready.");


  // Start the probes
  for (int i = 0; i < eeprom_numberofsensors; i++) {
    dht[i].setup(dhtpins[i]);
  }
  
  // Start preparing the initial screen!
  delay(500);
  lcd.clear();
  for ( int i = 0; i < eeprom_numberofsensors; i++ ) {
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
  // Make sure we have waited at least the minimum samping period for the DHT's
  // to return some usefull data
  delay(dht[0].getMinimumSamplingPeriod());
  // Request a temperature measurement from the OneWire Probes
  dallas.requestTemperatures();
  
  // Set all readings to the "No Sensor value"
  float temps[] = { -127, -127, -127, -127 };
  float hums[] = { -127, -127, -127, -127 };

  uint8_t hum;
  float temp;
  
  // Read all the DS18x20 and DHTxx sensors
  for (int i = 0; i < eeprom_numberofsensors; i++) {
    temp = float(dallas.getTempC(Probes[i]));
    if ( temp != 85 && temp != -127 ) {
      temps[i] = temp;
    }
    hum = dht[i].getHumidity();
    if ( isnan(hum) != 1 && hum != 0 ) {
      hums[i] = hum;
    }
  }
  
  /* Test value generation
  temps[0] = random(-10, 10);
  temps[1] = random(5, 15);
  temps[2] = random(20, 25);
  temps[3] = random(-20, 75);
  hums[0] = random(5, 25);
  hums[1] = random(35, 65);
  hums[2] = random(50, 75);
  hums[3] = random(5, 95);
  delay(1000);
  /* Uncomment above section for testing! */

  // Dump the sensor values as JSON on Serial
  Serial.print("{\"WeatherDuino\":[");
  for (int i = 0; i < eeprom_numberofsensors; i++) {
    Serial.print("{\"probe\":");
    Serial.print(int(i + 1));
    Serial.print(",\"temp\":");
    Serial.print(temps[i]);
    Serial.print(",\"humid\":");
    Serial.print(hums[i]);
    // Max 4 probes, should not place the comma after the last probe
    if ( i < eeprom_numberofsensors - 1 ) {
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
    } else if ( hums[i] >= 99 ) {
      lcd.print("99");
    } else {
      lcd.print(hums[i],0);
    }
  }
}

void find_onewire_devices(){
  // loops trough the list of connected devices and registers them to the eeprom
  // this only happens when 0 sensors have been found on setup
  // this happens only on the first boot
  // New sensors should be attached starting at sensor 1, etc.
  Serial.println("Detecting new devices, connect one at a time");
  byte type_s;
  byte addr[8];
  byte number_of_sensors = 0;
  bool foundmatch = false;
  bool foundnew = true;

  while(true){
    if (number_of_sensors == 4){
      Serial.println("Max number of sensors attached.");
      return;
    }
    if ( !oneWire.search(addr)) {
      // nothing left to find, lets wait for a new device to be attached
      delay(1000);
      Serial.println("waiting for reboot or new sensors to be attached.");
    } else {
      // asume the found device is new
      foundnew = true;
      for(int i = 0; i < number_of_sensors; i++) {
        // assume we are matching with the current eeprom reading
        foundmatch = true;
        for(int j = 0; j < 8; j++) {
          if(Probes[i][j] != addr[j]){
            // we are not matching, thus break the loop, and bubble up that this
            // is not a match, ergo, it's still a new sensor
            foundmatch = false;
            break;
          }
        }
        // all octecs are the same since we dind't break and there's a match,
        // so not new
        if(foundmatch){
          foundnew = false;
          break;
        }
        // still new if we reached this far, lets continue the loop over all
        // sensors in eeprom
        foundnew = true;
      }
      // we checked all sensors in eeprom, is the search result still new?
      if(foundnew){
        // addr should now contain a onewire addr
        switch (addr[0]) {
          case 0x10:
            Serial.println("  Chip = DS18S20 found");  // or old DS1820
            type_s = 1;
            break;
          case 0x28:
            Serial.println("  Chip = DS18B20 found");
            type_s = 0;
            break;
          case 0x22:
            Serial.println("  Chip = DS1822 found");
            type_s = 0;
            break;
          default:
            Serial.println("Device found, but not a DS18x20 family device.");
            return;
        }
        // found an address, lets store it in eeprom
        for(int i = 0; i < 8; i++) {
          EEPROM.write(eeprom_addressoffset + (8*number_of_sensors) + i, addr[i]);
          Probes[number_of_sensors][i] = addr[i];
          foundnew = false;
        }
        number_of_sensors++;
        EEPROM.write(eeprom_sensorcountoffset, number_of_sensors);
      }
    }
  }
}

void read_onewire_addresses(){
  for (int i = 0; i < eeprom_numberofsensors; i++) {
    for(int j = 0; j < 8; j++) {
      Probes[i][j] = EEPROM.read(eeprom_addressoffset + (8*i) + j);
    }
  }
}
