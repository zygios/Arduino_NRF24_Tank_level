#define DEBUG

#ifdef DEBUG
  #define debug(x)     Serial.print(x)
  #define debugln(x)   Serial.println(x)
#else
  #define debug(x)     // define empty, so macro does nothing
  #define debugln(x)
#endif

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <Wire.h> 

//DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 6
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
#define NUM_SENSORS 6
#define TEMPERATURE_PRECISION 9
int temperature[NUM_SENSORS];
DeviceAddress sensor_address[NUM_SENSORS] =
{
    { 0x28, 0x95, 0xF3, 0x44, 0x06, 0x00, 0x00, 0x93 },       // Lauko
    { 0x28, 0xFF, 0x32, 0x34, 0x63, 0x16, 0x04, 0x6F },       // svetine
    { 0x28, 0xFF, 0xFD, 0x73, 0x63, 0x16, 0x03, 0xC7 },       // zidinys
    { 0x28, 0xFF, 0xB5, 0x36, 0x63, 0x16, 0x04, 0xF2 },       // garazas
    { 0x28, 0xFF, 0x65, 0x0F, 0x62, 0x16, 0x04, 0xDC },       // hidroforas
    {0x28, 0xEB, 0x92, 0x44, 0x06, 0x00, 0x00, 0xC4}  //palepe
};

//nRF24L01
RF24 radio(7,8);
const uint64_t pipes = 0xB3B4B5B6F1LL; // Needs to be the same for communicating between 2 NRF24L01 
const long RFUpdatePeriod =  60000L;  //60000=1min
long RFLastUpdate = 0;

typedef struct {
  int lauko; 
  int svetai; 
  int zidinys; 
  int garazas;
  int hidroforas;
  int palepe;
} dataPacket;

dataPacket packet;

void setup() {
  Serial.begin(115200);
  printf_begin();
  // Setup and configure rf radio
  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(0,15);                 // Smallest time between retries, max no. of retries
  //radio.setPayloadSize(2); // setting the payload size to the needed value
  radio.setPayloadSize(sizeof(dataPacket));
  radio.setDataRate(RF24_250KBPS); // reducing bandwidth
  radio.openWritingPipe(pipes);
  radio.printDetails();  
  //DS18B20 INIT
  Wire.begin();
  sensors.begin();
  for( int i=0; i<NUM_SENSORS; i++){
    sensors.setResolution( sensor_address[i], TEMPERATURE_PRECISION);
  }
  debugln("Setup done!!!");
}


void loop() {
  unsigned long currentMillis = millis();
  readTemp();
  if(currentMillis - RFLastUpdate > RFUpdatePeriod) {
    RFLastUpdate = currentMillis;
    packet.lauko = temperature[0];
    packet.svetai = temperature[1];
    packet.zidinys = temperature[2];
    packet.garazas = temperature[3];
    packet.hidroforas = temperature[4];
    packet.palepe = temperature[5];
    
    radio.stopListening();                                  // First, stop listening so we can talk.
    if (radio.write(&packet, sizeof(dataPacket) )){
      debugln(F("failed.")); 
    }else{
      debugln(F("Packed send!!."));
    }
  }
  delay(1000);
}

void readTemp(){
  sensors.requestTemperatures();
  for(int i=0; i<NUM_SENSORS; i++){
    if((int) sensors.getTempC(sensor_address[i]) == -127){
      temperature[i] = 0;
    }else{
      temperature[i] = (int) sensors.getTempC(sensor_address[i]);
      debugln(temperature[i]);
    }
    char TempBuffer[5];
    //sprintf(TempBuffer,"Temp_%d:%02d", i, temperature[i]);
    //debugln(TempBuffer);
  }
}
