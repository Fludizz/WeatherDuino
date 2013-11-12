#include <DHT.h>
#include <EtherCard.h>
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
const DeviceAddress Probe[4] PROGMEM = 
{
  { 0x28, 0x0B, 0x4D, 0xC6, 0x04, 0x00, 0x00, 0x4D }, // OneWire probe #1
  { 0x28, 0x35, 0xD8, 0xC6, 0x04, 0x00, 0x00, 0xD6 }, // OneWire probe #2
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, // OneWire probe #3
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }  // OneWire probe #4
};

// enc28j60 connections:
// SPI SCK 	 Pin D13 
// SPI S0 	 Pin D12 
// SPI CI 	 Pin D11 
// SPI CS 	 Pin D08
// ethernet interface mac address - must be unique on your network
// last three bytes of the MAC are used as ID header.
static byte mymac[] = { 0xAC,0xDE,0x48,0xC0,0xFF,0xEE };
byte Ethernet::buffer[700]; // Minimal value to get basic DHCP reply

// Set STATIC to 1 to disable DHCP
#define STATIC 0
// If set to Static, use below addresses
#if STATIC
  static byte myip[] PROGMEM = { 10,11,12,13 };
  static byte mymask[] PROGMEM = { 255,255,255,0 };
  static byte gwip[] PROGMEM = { 10,11,12,1 };
#endif
const int srcPort = 65000; // Originating port
const int dstPort = 65001; // Destination port
byte dstIp[] = { 255,255,255,255 }; // Destination IP address


void setup() {
//  Serial.begin(57600);
  //Set the ethernet interface
  ether.begin(sizeof Ethernet::buffer, mymac, 8); // Set the int to the Arduino Pin.
  #if STATIC
    ether.staticSetup(myip, mymask, gwip);
  #else
    while (!ether.dhcpSetup()) {
      // Keep trying to get DHCP address until we have one.
      delay(10000);
      ether.dhcpSetup();
    }
  #endif
  // Start the probes
  dht[0].begin();
  dht[1].begin();
  dht[2].begin();
  dht[3].begin();
  onewire.begin();
}

void loop() {
  // Request a temperature measurement from the OneWire Probes
  onewire.requestTemperatures();
  // Set all readings to the "No Sensor value"
  int16_t temps[] = { -12700, -12700, -12700, -12700 }; // multiply by 100 for 2 decimals accurate integers
  int16_t hums[] = { -127, -127, -127, -127 };
  int16_t hum;
  int16_t temp;

  // Read all the DS18B20 and DHT11 sensors
  for (int i = 0; i < 4; i++) {
    temp = int(onewire.getTempC(Probe[i]) * 100); // multiply by 100 to get 2 decimals accurate integers
    if ( temp != 8500 ) {
      temps[i] = temp;
    }
    hum = dht[i].readHumidity();
    if ( isnan(hum) != 1 && hum != 0 ) {
      hums[i] = hum;
    }
  }

  /* Test value generation
  temps[0] = 1004; //int(random(-20, 75) * 100);
  temps[1] = -1212; //int(random(-20, 75) * 100);
  temps[2] = -105; //int(random(-20, 75) * 100);
  temps[3] = 309; //int(random(-20, 75) * 100);
  hums[0] = random(5, 95);
  hums[1] = random(5, 95);
  hums[2] = random(5, 95);
  hums[3] = random(5, 95);
  delay(1000);
  /* Uncomment above section for testing! */

  /* Dump the sensor values as JSON in a var
  String json_out = "{\"WeatherDuino\":[";
  for (int i = 0; i < 4; i++) {
    json_out += "{\"probe\":";
    json_out += int(i + 1);
    json_out += ",\"temp\":";
    json_out += FloatToStr(temps[i]);
    json_out += ",\"humid\":";
    json_out += FloatToStr(hums[i]);
    // Max 4 probes, should not place the comma after the last probe
    if ( i < 3 ) {
      json_out += "},";
    }
  } 
  // To reduce Serial.Print() lines in the for loop, add the last
  // closing } for the last probe down here as well.
  json_out += "}]}";
  Serial.println(json_out);  
  */
  // Packet format: 
  // Byte 1, 2 and 3 are device ID
  // Bytes up to byte 0xFF are Probe 1
  // Bytes up to byte 0xFF are Probe 2
  // Bytes up to byte 0xFF are Probe 3
  // Bytes up to byte 0xFF are Probe 4
  byte packet_data[] = { mymac[3], mymac[4], mymac[5], 
                         char(temps[0]), 0xFF, char(hums[0]), 0xFF,
                         char(temps[1]), 0xFF, char(hums[1]), 0xFF,
                         char(temps[2]), 0xFF, char(hums[2]), 0xFF,
                         char(temps[3]), 0xFF, char(hums[3]), 0xFF};
  
//  int len = json_out.length()+1;
//  char packet_data[len];
//  json_out.toCharArray(packet_data, len);
  ether.sendUdp((char *) packet_data, 128, srcPort, dstIp, dstPort);
}

String FloatToStr(float value) {
  // Returns the float as a string with 2 decimals accuracy.
  int major = int(value);
  value = value * 100;
  int minor = int(value);
  minor = minor - major * 100;
  if (minor < 0 ) {
    // If the number is negative, make it positive.
    minor = minor * -1;
  }
  String result = "";
  result += major;
  result += ".";
  if ( minor < 10 ) {
    result += "0";
  }
  result += minor;
  return result;
}
  
    
