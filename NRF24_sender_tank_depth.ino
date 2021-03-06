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
#include "LowPower.h"

//Distance sensor
const byte trigPin = 4;
const byte echoPin = 3;
const byte DistVcc = 5;
const int TankDepth = 230; // tank depth in centimeters
long duration;
int distance;

//nRF24L01
RF24 radio(7,8);

const uint64_t pipes = 0xABCDABCD71LL; // Needs to be the same for communicating between 2 NRF24L01 

typedef struct {
  int dist; //distante
  int perc; //percentage
  int volt; //battery reading
} dataPacket;

dataPacket packet;

void setup() {
  Serial.begin(115200);
  printf_begin();
  pinMode(DistVcc, OUTPUT);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
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
  debugln("Setup done!!!");
  delay(300);
}

void burn8Readings(int pin)
{
  for (int i = 0; i < 8; i++)
  {
    analogRead(pin);
  }
}

void loop() {
  digitalWrite(DistVcc, HIGH);
  delay(300);
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance= duration*0.034/2;
  // Prints the distance on the Serial Monitor
  digitalWrite(DistVcc, LOW);
 
  printf("Distance: %d cm.\n\r",distance);
  printf("Uzpildyta: %d .\n\r",packet.perc);
  uint16_t nResult1, nResult2;

  analogReference(INTERNAL);    // set the ADC reference to 1.1V
  burn8Readings(A0);            // make 8 readings but don't use them to ensure good reading after ADC reference change
  delay(10);                    // idle some time
  nResult1 = analogRead(A0);    // read actual value

  analogReference(DEFAULT);     // set the ADC reference back to internal for other measurements
  burn8Readings(A0);            // make 8 readings but don't use them to ensure good reading after ADC reference change
  delay(10);                    // idle again
  nResult2 = analogRead(A0);    // do other measurements

// print result to serial interface..
  Serial.print("1: ");  
  Serial.print(nResult1);
  Serial.print(" - 2: ");
  Serial.println(nResult2);

  int anlVolt = analogRead(A0);
  debug("Battery:");
  debugln(anlVolt);
  packet.dist = distance;
  packet.perc = ((TankDepth-distance)*100)/TankDepth;
  packet.volt = nResult2;
  
  radio.stopListening();                                  // First, stop listening so we can talk.
  if (radio.write(&packet, sizeof(dataPacket) )){
    Serial.println(F("failed.")); 
  }
  delay(200);
  unsigned int sleepCounter;
  //37 = 5 min sleep
  for (sleepCounter =7; sleepCounter > 0; sleepCounter--){
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
  }
}
