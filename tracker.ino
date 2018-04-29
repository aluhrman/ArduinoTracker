#define GSMAPN "internet.itelcel.com"
#define GSMUSER "webgprs"
#define GSMPASSWORD "webgprs2002"

#include <Time.h>
#include <sim800Client.h>
#include <PubSubClient.h>
#include <TimeAlarms.h>
#include <TinyGPS++.h>


// The TinyGPS++ object
TinyGPSPlus gps;

sim800Client s800;
char imeicode[16];


// Update these with values suitable for your network.
// AWS Mosquitto Server
char server[] = "xx.xx.xx.xx";


void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
  char mypl[48];
  Serial.println(length);
  memcpy(mypl,payload,length);
  mypl[length]=char(0);
  Serial.print("receive: ");
  Serial.print(topic);
  Serial.print("->");
  Serial.println(mypl);
}

PubSubClient client(server, 1883, callback, s800);


void pub()
{
    
    if (!client.connect(imeicode)) {
    Serial.println("No MQTT Connection");
    Serial.println("Resetting modem");
  }
  else {
    Serial.println("MQTT Connection Available");
    };

  //s800.checkNetwork();
  smartDelay(1000);
  displayInfo();
}

void setup()
{
  
  Serial.begin(9600);
  Serial3.begin(9600);
  Serial.println("Starting Up Tracker");

  for (int i=0; i<10; i++){
    delay(5000);
    Serial.println("try to init modem");

#ifdef HARDWARESERIAL
    if (s800.init( 7, 6)) break;
#else
    if (s800.init(&Serial1 , 7, 6)) break;
#endif

  }
  
  Serial.println("try to setup modem");
  
  s800.setup();
  s800.stop();
  s800.TCPstop();
  s800.getIMEI(imeicode);
  Serial.print("IMEI: ");
  Serial.println(imeicode);


  while (!s800.TCPstart(GSMAPN,GSMUSER,GSMPASSWORD)) {
    Serial.println("TCPstart failed");
    s800.TCPstop();
    delay(1000);
  }
  Serial.println("TCPstart started");

  while (!client.connect(imeicode)) {
    Serial.println("connect failed");
    delay(1000);
  }

  Serial.println("Succesfully Connected!");
  client.publish("test/hello","Unit Booted Up");

  Alarm.timerRepeat(15, pub);             // Publish Timer

}

void loop()
{
      if (!client.connect(imeicode)) {
    Serial.println("loop - No MQTT Connection");
  }
  else {
    //Serial.println("loop - MQTT Connection Available");
    };
  // Keep MQTT Connection Alive
  client.loop();
  Alarm.delay(100); 

}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (Serial3.available())
    // Ensure GPS Serial Connection is Alive
      gps.encode(Serial3.read());
  } while (millis() - start < ms);
}

// Parse GPS and compose CSV message
void displayInfo()
{
  // Max message size of 128 Bytes we keep it at 120 chars to be safe
  char sendbuffer[120]; 
  if (gps.location.isValid()){
    
    float latitude = gps.location.lat();
    float longitude = gps.location.lng();
    float speed_kph = gps.speed.kmph();
    float altitude = gps.altitude.meters();

    char *p = sendbuffer;
    // add speed value
    dtostrf(speed_kph, 2, 2, p);
    p += strlen(p);
    p[0] = ','; p++;

    // concat latitude
    dtostrf(latitude, 2, 6, p);
    p += strlen(p);
    p[0] = ','; p++;

    // concat longitude
    dtostrf(longitude, 3, 6, p);
    p += strlen(p);
    p[0] = ','; p++;

    // concat altitude
    dtostrf(altitude, 2, 2, p);
    p += strlen(p);

    // null terminate
    p[0] = 0;

    Serial.println(sendbuffer);
    client.publish("test/gps",sendbuffer);
}}
