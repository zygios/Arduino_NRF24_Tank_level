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

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 
RF24 radio(46,47);
// Topology
const uint64_t pipes = 0xABCDABCD71LL;              // Radio pipe addresses for the 2 nodes to communicate.

// A single byte to keep track of the data being sent back and forth
typedef struct {
  int dist; //distante
  int perc; //percentage
  int volt; //battery reading
} RxMsg;

RxMsg packet;
char OledTxt[15];
unsigned int fVolt = 0;
unsigned int heaVolt = 0;
unsigned int decVolt = 0;

//LCD
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

//RTC
#include "RTClib.h"
#include <Wire.h>
RTC_DS1307 RTC;
DateTime dt_now;



void setup() {
  Serial.begin(115200);
  debugln(F("Starting init"));
  //RTC INIT
  Wire.begin();
  RTC.begin();
  if (! RTC.begin()) {
    debugln(F("Couldn't find RTC"));
    while (1);
  }
  if (RTC.isrunning()) {
    debugln(F("Nustatom RTC laika!!!"));
    //RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  lcd.begin();
  lcd.backlight();
  lcd.home (); 
  lcd.setCursor(0,0);
  lcd.print(F("Temperaturos/backos"));  
  lcd.setCursor(0,1);
  lcd.print(F("sistema v.1"));  
  Wire.begin();
  //NRF24 init
   printf_begin();
  // Setup and configure rf radio
  radio.begin();
  radio.setAutoAck(1);                    // Ensure autoACK is enabled
  radio.enableAckPayload();               // Allow optional ack payloads
  radio.setRetries(0,15);                 // Smallest time between retries, max no. of retries
  //radio.setPayloadSize(2);                // Here we are sending 1-byte payloads to test the call-response speed
  radio.enableDynamicPayloads();
  radio.setDataRate(RF24_250KBPS); // reducing bandwidth
  radio.openReadingPipe(1,pipes);
  radio.startListening();                 // Start listening
  radio.printDetails();  
  delay(1000);
  lcd.clear();
  debugln(F("Init complete"));
}

void loop() {
  dt_now = RTC.now();
  prtTime();
  while(radio.available() ){
    unsigned long tim = micros();
    radio.read( &packet, sizeof(RxMsg));
    //printf("Got response %d, round-trip delay: %lu microseconds\n\r",RX_msg,tim-time);
    debugln(packet.dist);
    debugln(packet.perc);
    debugln(packet.volt);
    debugln("-------");
    int tempVolt = packet.volt;
    //debug("tempVolt:");
    //debugln(tempVolt);
    fVolt = map(tempVolt, 0, 1023, 0, 853);
    heaVolt = (fVolt/100);
    decVolt = ((fVolt-heaVolt) % 100);
    //debug("decVolt:");
    //debugln(decVolt);
    lcd.setCursor(0,1);
    sprintf(OledTxt,"%d%%",packet.perc); 
    lcd.print(OledTxt);
    lcd.setCursor(0,2);
    sprintf(OledTxt,"Gylis:%d cm",packet.dist); 
    lcd.print(OledTxt);
    lcd.setCursor(0,3);
    sprintf(OledTxt,"Batt:%d.%02d V", heaVolt, decVolt); 
    lcd.print(OledTxt);
  }
  delay(300);
}

void prtTime(){
  char curr_TimeBuffer[4];
  sprintf(curr_TimeBuffer,"%02u:%02u",dt_now.hour(),dt_now.minute());  
  lcd.setCursor(0,0);
  lcd.print(curr_TimeBuffer);
}
