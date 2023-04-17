/*
 * Project TrashCanModule
 * Description: the trash can Module will take an ultrasonic reading to detect if it is full, this data will then be published either via Lora or Cell, single neopixel will indicate status 
 * Author: Arjun Bhakta/Thomas Abeyta
 */
#include "Adafruit_MQTT_SPARK.h"
#include "Adafruit_MQTT.h"
#include "Grove-Ultrasonic-Ranger.h"
#include "neopixel.h"
#include "TCreds.h"
const unsigned long SEND_INTERVAL_MS = 6000;
const size_t READ_BUF_SIZE = 64;
const int RADIOADDRESS = 102; //
const int RADIONETWORKID = 5; // range of 0-16
const int SENDADDRESS = 101; // address of radio to be sent to
char readBuf[READ_BUF_SIZE + 1];
// Global variables
unsigned long lastSend = 0;
String password = "FABC0002EEDCAA90FABC0002EEDCAA90"; // AES password
String Location = "PinoYards";

TCPClient TheClient;
Adafruit_MQTT_SPARK mqtt(&TheClient, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish trashCanFullness = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/smart-trash-can");

unsigned int mqttTimer;
int rangeInInches;
int fullness;


SYSTEM_MODE(SEMI_AUTOMATIC);

//UltraSonicSetup
Ultrasonic ultrasonic(A0);
int distance;
// int trashFullness;
unsigned int detectionTimer;
unsigned int loraTimer;


// Neo Pixel Setup
#define PIXEL_COUNT 1
#define PIXEL_PIN D12 
#define PIXEL_TYPE WS2812B
Adafruit_NeoPixel strip(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE); 

// Lora Setup Needs to be Added

enum TrashCanState{
  EMPTY,
  HALF_FULL,
  FULL
};

TrashCanState currentTrashCanState;

TrashCanState updateTrashCanState(int distance_){
    const int fullDistance = 5;
    const int halfDistance = 15;
    TrashCanState tempState = EMPTY;

    if(distance_ > halfDistance){
      return tempState= EMPTY;
    }
    else if( distance_ < halfDistance && distance_ > fullDistance ){
      return tempState = HALF_FULL;
    }
    else if( distance_ < fullDistance){
      return tempState = FULL;
    }
    return tempState;
}


void SetNeoPixelColor(TrashCanState state) {
  switch (state) {
    case EMPTY:
      for (int i = 0; i < PIXEL_COUNT; i ++) {
        strip.setPixelColor(i, 0, 255, 0);//Green
      } 
      strip.show();
      break;
    case HALF_FULL:
      for (int i = 0; i < PIXEL_COUNT; i ++) {
      strip.setPixelColor(i, 255, 255, 0);//Yellow
      }
      strip.show();
      break;
    case FULL:
      for (int i = 0; i < PIXEL_COUNT; i ++) {
      strip.setPixelColor(i, 255, 0, 0);//Red
      }
      strip.show();
      break;
    
    default:
      break;
  }
}

void setup() {
  Serial.begin(9600);
  waitFor(Serial.isConnected,10000);
  // setupEniviromentalSensor();
  strip.begin(); // initialize the neopixel strip
  strip.setBrightness(12);
  strip.show(); // turn off all neopixels
  Serial1.begin(115200);
  reyaxSetup(password);

 // Connect to WiFi without going to Particle Cloud
    WiFi.connect();
    while (WiFi.connecting()) {
        Serial.printf(".");
    }

    MQTT_connect();

   
}

void loop() {

  // read ultrasonic Sensor, change trashCanStates, update neoPixel
  if( millis()- detectionTimer > 1000){

    distance = getUltraSonicData();
    currentTrashCanState = updateTrashCanState(distance);
    SetNeoPixelColor(currentTrashCanState);

    detectionTimer = millis();
  }
  if ((millis() - mqttTimer > 60000)) {
    fullness = getUltraSonicData();
    sendMQTT();
    mqttTimer = millis();
  }
 
      
  // // timer to send Lora Data
  // if ( millis() - loraTimer > 60'000 ){
  //     Serial.println(" Sending state via Lora");
  //     sendData(rangeInInches);
  //     // resetF();
  //     //readData();
  //     for (int i = 0; i < PIXEL_COUNT; i ++) {
  //       strip.setPixelColor(i, 0, 0, 255); // Blue
  //     }
  //     strip.show();
  //     delay(3000);
  //      loraTimer= millis();
  // }
}



int getUltraSonicData(){
	int rangeInInches = ultrasonic.MeasureInInches();
  Serial.printf(" %i \n",rangeInInches );
  return rangeInInches;
}

//LoRa code
// void sendData( rangeInInches) {
//     char buffer[60];
//     sprintf(buffer, "AT+SEND=%i,60,%s,%s\r\n", SENDADDRESS,String(trashCanFullness).c_str(), Location.c_str());
//     Serial1.printf("%s",buffer);
//     if (Serial1.available() > 0) {
//       Serial.printf("Awaiting Reply from send\n");
//       String reply = Serial1.readStringUntil('\n');
//       Serial.printf("LoRa Module: %s\n", reply.c_str());
//   }
// }

// void readData() {
//  if (Serial1.available() > 0) {
//       Serial.printf("Awaiting Reply from send\n");
//       String reply = Serial1.readStringUntil('\r\n');
//       if (reply.startsWith("+RCV=101")) {
//         Serial.printf("LoRa Module: %s\n", reply.c_str());
//       }
//  }
// }
void resetF(){
  String reply; // string to store replies from module
  Serial1.printf("AT+FACTORY\r\n");
  delay(300);
  if (Serial1.available() > 0) {
    Serial.printf("Awaiting Reply from reset\n");
    reply = Serial1.readStringUntil('\n');
    Serial.printf("Reply from reset: %s\n", reply.c_str());
  }
}
void reyaxSetup(String password) {
  const int SPREADINGFACTOR = 10;
  const int BANDWIDTH = 7;
  const int CODINGRATE = 1;
  const int PREAMBLE = 7;
  int delay_MS = 300;
  String reply; // string to store replies from module
  Serial1.printf("AT+ADDRESS=%i\r\n", RADIOADDRESS);
  delay(delay_MS);
  if (Serial1.available() > 0) {
    Serial.printf("Awaiting Reply from address\n");
    reply = Serial1.readStringUntil('\n');
    Serial.printf("Reply address: %s\n", reply.c_str());
  }
  Serial1.printf("AT+NETWORKID=%i\r\n", RADIONETWORKID);
  delay(delay_MS);
  if (Serial1.available() > 0) {
    Serial.printf("Awaiting Reply from networkid\n");
    reply = Serial1.readStringUntil('\n');
    Serial.printf("Reply network: %s\n", reply.c_str());
  }
  Serial1.printf("AT+CPIN=%s\r\n", password.c_str());
  delay(delay_MS);
  if (Serial1.available() > 0) {
    Serial.printf("Awaiting Reply from password\n");
    reply = Serial1.readStringUntil('\n');
    Serial.printf("password Reply: %s\n", reply.c_str());
  }
  Serial1.printf("AT+PARAMETER=%i,%i,%i,%i\r\n", SPREADINGFACTOR, BANDWIDTH, CODINGRATE, PREAMBLE);
  delay(delay_MS);
  if (Serial1.available() > 0) {
    Serial.printf("Awaiting Reply for parameters\n");
    reply = Serial1.readStringUntil('\n');
    Serial.printf("parameter reply: %s\n", reply.c_str());
  }
  Serial1.printf("AT+ADDRESS?\r\n");
  delay(delay_MS);
  if (Serial1.available() > 0) {
    Serial.printf("Awaiting Reply from network\n");
    reply = Serial1.readStringUntil('\n');
    Serial.printf("Radio Address: %s\n", reply.c_str());
  }
  Serial1.printf("AT+NETWORKID?\r\n");
  delay(delay_MS);
  if (Serial1.available() > 0) {
    Serial.printf("Awaiting Reply from networkid\n");
    reply = Serial1.readStringUntil('\n');
    Serial.printf("Radio Network: %s\n", reply.c_str());
  }
  Serial1.printf("AT+PARAMETER?\r\n");
  delay(300);
  if (Serial1.available() > 0) {
    Serial.printf("Awaiting Reply from Parameter\n");
    reply = Serial1.readStringUntil('\n');
    Serial.printf("Radio Parameters: %s\n", reply.c_str());
  }
  Serial1.printf("AT+CPIN?\r\n");
  delay(delay_MS);
  if (Serial1.available() > 0) {
    Serial.printf("Awaiting Reply cpin\n");
    reply = Serial1.readStringUntil('\n');
    Serial.printf("Radio Password: %s\n", reply.c_str());
  }
  Serial1.printf("AT+CRFOP?\r\n"); // RF output power
  delay(delay_MS);
  if (Serial1.available() > 0) {
    Serial.printf("Awaiting Reply from RF output power:\n");
    reply = Serial1.readStringUntil('\n');
    Serial.printf("Reply RF output power: %s\n\n\n", reply.c_str());
  }
}


void sendMQTT(){
        if (mqtt.connected()) {
            trashCanFullness.publish(fullness );
            Serial.printf("publish to online dashboard\n");
          for (int i = 0; i < PIXEL_COUNT; i ++) {
            strip.setPixelColor(i, 0, 0, 255); // Blue
          }
          strip.show();
          delay(3000);
        }
}

void MQTT_connect() {
    int8_t ret;

    // Stop if already connected.
    if (mqtt.connected()) {
        return;
    }
    Serial.print("Connecting to MQTT... ");
    while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
        Serial.printf("%s\n", (char *)mqtt.connectErrorString(ret));
        Serial.printf("Retrying MQTT connection in 5 seconds..\n");
        mqtt.disconnect();
        delay(5000); // wait 5 seconds
    }
    Serial.printf("MQTT Connected!\n");
}