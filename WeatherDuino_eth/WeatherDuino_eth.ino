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
DHT dht[4] = {
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

static byte probecount = 4;

// Add all four probes to an array for easy for-loop processing!
DeviceAddress Probes[] = 
{
  { 0x28, 0x0D, 0x29, 0xDA, 0x04, 0x00, 0x00, 0xDB }, // OneWire probe #1
  { 0x28, 0x44, 0x1F, 0xD3, 0x04, 0x00, 0x00, 0xB3 }, // OneWire probe #2
  { 0x28, 0xAC, 0x89, 0xD3, 0x04, 0x00, 0x00, 0x10 }, // OneWire probe #3
  { 0x28, 0x4B, 0x5A, 0xD3, 0x04, 0x00, 0x00, 0xAC }  // OneWire probe #4
};

// enc28j60 connections:
// SPI SCK 	 Pin D13 
// SPI S0 	 Pin D12 
// SPI CI 	 Pin D11 
// SPI CS 	 Pin D08
const byte chipSelect = 8;

// ethernet interface mac address - must be unique on your network
// last three bytes of the MAC are used as ID header.
const byte mymac[] = { 0xAC,0xDE,0x48,0xC0,0xFF,0xEE };
byte Ethernet::buffer[700]; // Minimal value to get basic DHCP reply

// Set STATIC to 1 to disable DHCP
#define STATIC 1
// If set to Static, use below addresses
#if STATIC
  const byte myip[] PROGMEM = { 192,168,1,2 };
  const byte mymask[] PROGMEM = { 255,255,255,0 };
  const byte gwip[] PROGMEM = { 192,168,1,1 };
#endif
const int srcPort = 65000; // Originating port
const int dstPort = 65001; // Destination port
uint8_t dstIp[] = { 255,255,255,255 }; // Destination IP address

void setup() {
  Serial.begin(57600);
  //Set the ethernet interface
  if (ether.begin(sizeof Ethernet::buffer, mymac, chipSelect) == 0)
    Serial.println("No eth?"); 

  #if STATIC
    ether.staticSetup(myip, mymask, gwip);
  #else
    while (!ether.dhcpSetup()) {
      // Keep trying to get DHCP address until we have one.
      delay(10000);
      ether.dhcpSetup();
    }
  #endif
  ether.printIp("IPv4: ", ether.myip);

  // Start the probes
  for (int i = 0; i < probecount; i++) {
    dht[i].begin();
  }
  onewire.begin();
}

void loop() {
  // Request a temperature measurement from the OneWire Probes
  onewire.requestTemperatures();

  // first byte is number of probes
  // subsequent pairs of 3 bytes consist of:
  // temperature, temperature behind decimal, humidity
  
  uint8_t packet_data[(probecount)*3 + 1];
  packet_data[0] = probecount;
  
  uint8_t hum;
  float temp;

  // Read all the DS18B20 and DHT* sensors
  for (int i = 0; i < probecount; i++) {
    temp = float(onewire.getTempC(Probes[i]));
    if ( temp < 100 ) {
      packet_data[(i*3)+1] = int(temp);
      packet_data[(i*3)+2] = int((temp - int(temp)) * 100 ); //ohgod help me fix this
    } else {
      packet_data[(i*3) + 1] = 0xffff; //max out values so listener knows its not valid
    }
    hum = dht[i].readHumidity();
    if (isnan(hum) != 1 && hum != 0 ) {
      packet_data[(i*3)+3] = hum;
    } else {
      packet_data[(i*3)+3] = 0xff; // max out values so listener knows its not valid
    }    
  }
  ether.sendUdp((char *) packet_data, sizeof(packet_data), srcPort, dstIp, dstPort);
}
