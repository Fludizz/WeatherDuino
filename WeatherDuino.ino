#include <DHT.h>
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>


#define DHTPIN 2     // what pin we're connected to
// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11 
//#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
DHT dht(DHTPIN, DHTTYPE);

// Connections:
// rs (LCD pin 4) to Arduino pin 12
// rw (LCD pin 5) to Arduino pin 11
// enable (LCD pin 6) to Arduino pin 10
// LCD pin 15 to Arduino pin 13
// LCD pins d4, d5, d6, d7 to Arduino pins 7, 6, 5, 4
LiquidCrystal lcd(12, 11, 10, 7, 6, 5, 4);

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 3

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
DeviceAddress Probe01 = { 0x28, 0x39, 0x31, 0xEA, 0x03, 0x00, 0x00, 0x9A }; 
DeviceAddress Probe02 = { 0x28, 0x11, 0x35, 0xEA, 0x03, 0x00, 0x00, 0x92 };


void setup() {
  Serial.begin(57600); 
  Serial.println("[WeatherDuino]");

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
  delay(1000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("In  Hum.:");
  lcd.setCursor(0,1);
  lcd.print("In  Temp:");
  lcd.setCursor(-4,2);
  lcd.print("Out Hum.:");
  lcd.setCursor(-4,3);
  lcd.print("Out Temp:");
  
  dht.begin();
  sensors.begin();
}

void loop() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float hin = dht.readHumidity();
  float hout = 0;
  sensors.requestTemperatures();
  float tin = sensors.getTempC(Probe01);
  float tout = sensors.getTempC(Probe02);

  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(tin) || isnan(hin)) {
    Serial.println("Failed to read from DHT");
  } else {
    Serial.print("H-in: "); 
    Serial.print(hin);
    Serial.print("\tT-in: "); 
    Serial.print(tin);
    Serial.print("\tH-out: "); 
    Serial.print(hout);
    Serial.print("\tT-out: "); 
    Serial.println(tout);
    lcd.setCursor(10,0);
    lcd.print(int(hin));
    lcd.print(" %  ");
    lcd.setCursor(10,1);
    lcd.print(int(tin));
    lcd.print(" ");
    lcd.print(char(223));
    lcd.print("C");
    lcd.setCursor(6,2);
    lcd.print(int(hout));
    lcd.print(" %  ");
    lcd.setCursor(6,3);
    lcd.print(int(tout));
    lcd.print(" ");
    lcd.print(char(223));
    lcd.print("C");
  }
}
