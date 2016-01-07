#include <EEPROM.h>
#include <EtherCard.h>
#include <DallasTemperature.h>
#include <DHT.h>
// lets include https://github.com/markruys/arduino-DHT/blob/master/DHT.cpp
#include <OneWire.h>

// DHT sensor settings
// Connect DHT pin 1 (on the left) of the sensor to +5V
// Connect DHT pin 2 of the sensor to whatever Datapin you choose
// Connect DHT pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
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
DeviceAddress Probes[4] = { //initialize to zero
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0}
};;

// enc28j60 connections:
// SPI SCK 	 Pin D13 
// SPI S0 	 Pin D12 
// SPI CI 	 Pin D11 
// SPI CS 	 Pin D08
const byte chipSelect = 8;

// ethernet interface mac address - must be unique on your network
// last three bytes of the MAC are used as ID header.
byte etherMac[6] = { 0xAC,0xDE,0x48,0xC0,0xFF,0x00 };
byte Ethernet::buffer[700]; // Minimal value to get basic DHCP reply
static BufferFiller bfill;

const char http_OK[] PROGMEM =
    "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n\r\n";

const int srcPort = 65000; // Originating port
const int dstPort = 65001; // Destination port
uint8_t dstIp[] = { 255,255,255,255 }; // Broadcast IP address

// store onewire addresses from the 16th bit in eeprom
const byte eeprom_IP = 0; // to 4
const byte eeprom_MAC = 4; // to 10
const byte eeprom_MAC_Random = 11; // to 12
const byte eeprom_sensorcountoffset = 12; // to 14
const byte eeprom_addressoffset = 16; // to 16+(4*8)

// how many bytes before we start with the sensor data in outbound packages
const byte UDPoffset = 6;
int eeprom_numberofsensors;
byte mac[6];

void setup() {
  Serial.begin(57600);

  eeprom_numberofsensors = EEPROM.read(eeprom_sensorcountoffset);
  Serial.print("Sensors stored in EEPROM: ");
  Serial.println(eeprom_numberofsensors);

  dallas.begin();
  if (eeprom_numberofsensors > 4 ||
      eeprom_numberofsensors == 0 ||
      eeprom_numberofsensors != dallas.getDeviceCount()){
    // we should trigger a scan, the number of probes has changed!
    EEPROM.write(eeprom_sensorcountoffset, 0);
    eeprom_numberofsensors = 0;
    find_onewire_devices();
  } else {
    read_onewire_addresses();
  }
  Serial.println("Sensors read from EEPROM, ready for network:");
  // Start the probes
  for (int i = 0; i < eeprom_numberofsensors; i++) {
    dht[i].setup(dhtpins[i]);
  }
  start_lan();
}

void start_lan(){
  // Set up the ethernet module with our MAC and static IP.
  readMAC(mac);
  if(mac[0] == 0 || mac[0] == 255){
    // we are using the firmware supplied MAC
    byte randommac = EEPROM.read(eeprom_MAC_Random);
    if(randommac != 0 && randommac != 255){
      etherMac[6] = randommac;
    } else {
      // No randomized mac stored yet, lets create and store
      etherMac[6] = random(1, 254);
      EEPROM.write(eeprom_MAC_Random, etherMac[6]);
    }

    if (ether.begin(sizeof Ethernet::buffer, etherMac, chipSelect) == 0){
      Serial.println("No eth?");
    }
    Serial.println("using default mac");
  } else {
    if (ether.begin(sizeof Ethernet::buffer, mac, chipSelect) == 0){
      Serial.println("No eth?");
    }
    Serial.println("using eeprom mac");
  }

  byte etherIP[4];
  readIP(etherIP);
  bool online = false;
  if (etherIP[0] == 0 || etherIP[0] == 255){
    Serial.println("Setting up DHCP");
    if (!ether.dhcpSetup()){
      Serial.println( "DHCP failed");
    } else {
      Serial.print("Online at DHCP:");
      ether.printIp("IP: ", ether.myip);
      online = true;
    }
  }
  if(!online){
    Serial.print("Online at STATIC:");
    ether.staticSetup(etherIP);
    ether.printIp("IP: ", ether.myip);
  }
}

void loop() {
  // Make sure we have waited at least the minimum samping period for the DHT's
  // to return some usefull data
  delay(dht[0].getMinimumSamplingPeriod());

  // Request a temperature measurement from the OneWire Sensors
  dallas.requestTemperatures();

  // first byte contains the protocol identifier
  // seccond byte contains the protocol version
  // third to 5th byte contain the last 3 mac address bytes
  // 6th byte is number of sensors
  // subsequent pairs of 5 bytes consist of:
  // 4 bytes temperature, humidity
  uint8_t packet_data[(eeprom_numberofsensors)*5 + UDPoffset];
  packet_data[0] = 0x65; // magic id for our protocol
  packet_data[1] = 0x02; // protocol version
  packet_data[2] = mac[3]; // device identifier
  packet_data[3] = mac[4];
  packet_data[4] = mac[5];
  packet_data[5] = eeprom_numberofsensors; // number of probes in this packet

  uint8_t hum;
  float temp;

  // Read all the DS18B20 and DHT* sensors
  for (int i = 0; i < eeprom_numberofsensors; i++) {
    Serial.print("Probe ");
    Serial.print(i);
    Serial.print(": ");
    
    temp = float(dallas.getTempC(Probes[i]));
    hum = dht[i].getHumidity();
    if (isnan(hum) == 1 || hum == 0 ) {
      hum = 0xff;
    } else {
      Serial.print(hum);
      Serial.print("%");
    }  
    packet_data[(i*5)+UDPoffset+4] = hum;
    
    if ( temp < 100 && temp != -127) {
      byte * tempbytes = (byte *) &temp; 
      packet_data[(i*5)+UDPoffset] = tempbytes[0];
      packet_data[(i*5)+UDPoffset+1] = tempbytes[1];
      packet_data[(i*5)+UDPoffset+2] = tempbytes[2];
      packet_data[(i*5)+UDPoffset+3] = tempbytes[3];
      //packet_data[(i*5)+UDPoffset+1] = int((temp - int(temp)) * 100 ); //ohgod help me fix this
      if(hum != 0xff){
        Serial.print(", ");
      }
      Serial.print(temp);
      Serial.print("c");
    } else {
      packet_data[(i*5)+UDPoffset] = 0xffffffff; //max out values so listener knows its not valid
    }
    Serial.println("");
  }
  ether.sendUdp((char *) packet_data, sizeof(packet_data), srcPort, dstIp, dstPort);
  delay(1000);
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

void readIP(byte ip[]){
  for(int i=0; i<4; ++i){
    ip[i] = EEPROM.read(i+eeprom_IP);
  }
}

void readMAC(byte mac[]){
  for(int i=0; i<6; ++i){
    mac[i] = EEPROM.read(i+eeprom_MAC);
  }
}

void setIP(byte ip[]){
  Serial.print("IP stored as: ");
  for(int i=0; i<4; ++i){
    EEPROM.write(i+eeprom_IP, ip[i]);
    Serial.print(ip[i], DEC);
    if (i < 3)
      Serial.print('.');
  }
  Serial.println();
}

void setMAC(byte mac[]){
  Serial.print("MAC stored as: ");
  for(int i=0; i<6; ++i){
    EEPROM.write(i+eeprom_MAC, mac[i]);
    Serial.print(mac[i], HEX);
    if (i < 5)
      Serial.print(':');
  }
  Serial.println();
}
