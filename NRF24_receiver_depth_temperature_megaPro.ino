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
#include <DHT12.h>
#include "RTClib.h"

// Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 
RF24 radio(46,47);
// Topology
//const uint64_t pipes = 0xABCDABCD71LL;              // Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[] = {0xABCDABCD71LL, 0xB3B4B5B6F1LL};
const long RFUpdatePeriod =  600000L;  //300000=5min 600000=10min
long RFLastUpdate = 0;

// A single byte to keep track of the data being sent back and forth
typedef struct {
  int dist; //distante
  int perc; //percentage
  int volt; //battery reading
} RxMsg;
RxMsg packet;

typedef struct {
  int lauko; 
  int svetai; 
  int zidinys; 
  int garazas;
  int hidroforas;
  int palepe;
} TempMsg;
TempMsg packet1;

char LCDTxt[20];
unsigned int fVolt = 0;
unsigned int heaVolt = 0;
unsigned int decVolt = 0;

//LCD
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
#define button_LCD 19
byte backlightState = 0;
const char* LCDLine0 = "%02u:%02u    %s\xDF""C/%02d%%";
const char* LCDLine1 = "Back:%03dcm %02d%% %d.%02dV";
const char* LCDLine1Error = "Back:nera duom. 10m ";
const char* LCDLine2 = "Lauk Sv Zi Ga Hi Pa ";
const char* LCDLine3 = "%02d %02d %02d %02d %02d %02d";
char str_temp[6];

//RTC
RTC_DS1307 RTC;
DateTime dt_now;
const long Temp_Period =  60000L;  //30000=30sec 
long previousMillis = 0;

//DHT12
DHT12 dht12;
float dht_temp;
int dht_hum;
const float dht_adj = 3.5;


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
  radio.openReadingPipe(0,pipes[0]);
  radio.openReadingPipe(1,pipes[1]);
  radio.startListening();                 // Start listening
  radio.printDetails();  
  pinMode(button_LCD, INPUT_PULLUP);
  dht12.begin();
  delay(1000);
  lcd.clear();
  debugln(F("Init complete"));
  readDHT12();
}

void readDHT12(){
  if(backlightState == 0){
    dht_temp = dht12.readTemperature() - dht_adj;
  }else{
    dht_temp = dht12.readTemperature();
  }
  dht_hum  = dht12.readHumidity();
  debugln("Temp: "+String(dht_temp)+ " , Hum: " + String(dht_hum));
}

void loop() {
  dt_now = RTC.now();
  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis > Temp_Period) {
    previousMillis = currentMillis;
    readDHT12();
  }
  backlightState = digitalRead(button_LCD);
  if(backlightState == 0){
    lcd.backlight();
  }else{
    lcd.noBacklight();
  }
  uint8_t pipe;
  while(radio.available(&pipe) ){
    unsigned long tim = micros();
    //reading packet from Tank
    delay(50);
    
    
    
    //printf("Got response %d, round-trip delay: %lu microseconds\n\r",RX_msg,tim-time);
    if(pipe == 0){
      radio.read( &packet, sizeof(RxMsg));
      Serial.println("Gautas paketas: " +String(sizeof(RxMsg)));
      debugln(packet.dist);
      debugln(packet.perc);
      debugln(packet.volt);
      debugln("-------");
      int tempVolt = packet.volt;
      debug("tempVolt:");
      debugln(tempVolt);
      fVolt = map(tempVolt, 0, 1023, 0, 803);
      heaVolt = (fVolt/100);
      decVolt = ((fVolt-heaVolt) % 100);
      //debug("decVolt:");
      //debugln(decVolt);
      RFLastUpdate = millis();
    }
    
    delay(100);
    //reading packet from Temperature
    if(pipe == 1){
      radio.read( &packet1, sizeof(TempMsg));
      Serial.println("Gautas paketas: " +String(sizeof(TempMsg)));
      debugln(packet1.lauko);
      debugln(packet1.palepe);
      debugln(packet1.svetai);
      debugln(packet1.zidinys);
      debugln(packet1.garazas);
      debugln(packet1.hidroforas);
   }
  }

  //output to LCD
  //const char* LCDLine0 = "%02u:%02u  %d.%02d \xB0C/%d%%";
  lcd.setCursor(0,0);
  dtostrf(dht_temp, 4, 2, str_temp);
  sprintf(LCDTxt,LCDLine0,dt_now.hour(),dt_now.minute(), str_temp, dht_hum); 
  lcd.print(LCDTxt);
  if(currentMillis - RFLastUpdate < RFUpdatePeriod){
    //const char* LCDLine1 = "Back:%03dcm %02d%% %d.%02dV";
    sprintf(LCDTxt,LCDLine1,packet.dist, packet.perc, heaVolt, decVolt); 
    lcd.setCursor(0,1);
    lcd.print(LCDTxt);
  }else{
    lcd.setCursor(0,1);
    lcd.print(LCDLine1Error);
  }
  lcd.setCursor(0,2);
  lcd.print(LCDLine2);

  //const char* LCDLine2 = "Lauk Sv Zi Ga Hi Pa ";
  sprintf(LCDTxt,LCDLine3,packet1.lauko, packet1.svetai, packet1.zidinys, packet1.garazas, packet1.hidroforas, packet1.palepe); 
  lcd.setCursor(0,3);
  lcd.print(LCDTxt);
  
  delay(50);
}
