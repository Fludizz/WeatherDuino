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
  
// LCD Panel Connections:
// rs (LCD pin 4) to Arduino pin 12
// rw (LCD pin 5) to Arduino pin 11
// enable (LCD pin 6) to Arduino pin 10
// LCD pins d4, d5, d6, d7 to Arduino pins 9, 8, 7, 6
LiquidCrystal lcd(12, 11, 10, 9, 8, 7, 6);

// Data wire is plugged into port 2 on the Arduino
// Setup a oneWire instance to communicate with any OneWire devices (not just 
// Maxim/Dallas temperature ICs)
OneWire oneWire(2);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature onewire(&oneWire);
DeviceAddress Probe01 = { 0x28, 0x0B, 0x4D, 0xC6, 0x04, 0x00, 0x00, 0x4D }; 
DeviceAddress Probe02 = { 0x28, 0x35, 0xD8, 0xC6, 0x04, 0x00, 0x00, 0xD6 };


void setup() {
  Serial.begin(57600); 
  lcd.begin(16,4);              // columns, rows.  use 16,2 for a 16x2 LCD, etc.
  lcd.clear();                  // start with a blank screen
  lcd.setCursor(0,0);           // set cursor to column 0, row 0 (the first row)
  lcd.print("WeatherDuino!");
  lcd.setCursor(0,1);
  lcd.print("v0.01");
  lcd.setCursor(-4,2);          // Line 3 and Line 4 are off by 4 chars.
  lcd.print("by Fludizz");
  lcd.setCursor(-4,3);
  lcd.print("DHT11 & DS18B20");
  delay(1000);                  // Show bootscreen for 1 second!
  
  // Start preparing the initial screen!
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Probe01:");
  lcd.setCursor(-4,2);
  lcd.print("Probe02:");
  
  // Start all the probes!
  dht1.begin();
  dht2.begin();
  onewire.begin();
}

void loop() {
  // Request a temperature measurement from the OneWire Probes
  onewire.requestTemperatures();
  // Read humidity from each DHT sensor (DHT is a very slow sensor)
  float hin = dht1.readHumidity();
  float hout = dht2.readHumidity();
  // Read temperatures from each OneWire Probe.
  float tin = onewire.getTempC(Probe01);
  float tout = onewire.getTempC(Probe02);
  
  // Check if returns are valid, if they are NaN (not a number) then something went wrong!
  // Note: DHT11 sometimes returns 0 instead of NaN if it fails to read the probe.
  Serial.print("{\"WeatherDuino\":[{\"probe\":1,\"temp\":"); 
  Serial.print(tin);
  Serial.print(",\"humid\":");
  if ( isnan(hin) || hin == 0 ) {
    Serial.print(-127.00);
  } else {
    Serial.print(hin);
  }
  Serial.print("},{\"probe\":2,\"temp\":");
  Serial.print(tout);
  Serial.print(",\"humid\":");
  if ( isnan(hout) || hout == 0 ) {
    Serial.print(-127.00);
  } else {
    Serial.print(hout);
  }
  Serial.println("}]}");

  // LCD line 2
  lcd.setCursor(1,1);
  if ( tin < -99 || tin > 99 ) {
    lcd.print("--.-");
  } else if ( tin < 10 && tin > 0 ) {
    lcd.print(" ");
    lcd.print(tin, 1);
  } else {
    lcd.print(tin, 1);
  }
  lcd.print(char(223));
  lcd.print("C ");
  lcd.setCursor(9,1);
  if ( isnan(hin) || hin == 0 ) {
    lcd.print("--");
  } else if ( hin < 10 ) {
    lcd.print(" ");
    lcd.print(hin, 0);
  } else {
    lcd.print(hin, 0);
  }
  lcd.print("% ");

  // LCD Line 4
  lcd.setCursor(-3,3);
  if ( tout < -99 || tout > 99 ) {
    lcd.print("--.-");
  } else if ( tout < 10 && tout > 0 ) {
    lcd.print(" ");
    lcd.print(tout, 1);
  } else {
    lcd.print(tout, 1);
  }
  lcd.print(char(223));
  lcd.print("C ");
  lcd.setCursor(5,3);
  if ( isnan(hout) || hout == 0 ) {
    lcd.print("--");
  } else if ( hout < 10 ) {
    lcd.print(" ");
    lcd.print(hout, 0);
  } else {
    lcd.print(hout, 0);
  }
  lcd.print("% ");
}
