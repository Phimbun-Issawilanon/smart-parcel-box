#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include "DFRobotDFPlayerMini.h"
#include <HardwareSerial.h>

#include <time.h>
#include <IOXhop_FirebaseESP32.h>
#include <ArduinoJson.h>

#define LINE_TOKEN  "xxxx"  //line token
#include <TridentTD_LineNotify.h>

#include <ESP32Servo.h>

const char* ssid = "xxxx"; //wifi name
const char* password = "xxxx"; //password
#define FIREBASE_HOST "xxxx"
#define FIREBASE_AUTH "xxxx"
const char* mqtt_server = "broker.netpie.io";
const int mqtt_port = 1883;
const char* mqtt_client = "xxxx";
const char* mqtt_username = "xxxx";
const char* mqtt_password = "xxxx";
char ntp_server1[20] = "pool.ntp.org";
char ntp_server2[20] = "time.nist.gov";
char ntp_server3[20] = "time.uni.net.th";

int timezone = 7*3600;
int dst = 0;

const int trig = 27; //ประกาศขา trig
const int echo = 26; //ประกาศขา echo

long duration, distance; //ประกาศตัวแปรเก็บค่าระยะ

Servo myservo; //ประกาศตัวแปรแทน Servo
WiFiClient espClient;
PubSubClient client(espClient);

HardwareSerial mySoftwareSerial(1);

DFRobotDFPlayerMini myDFPlayer;
void reconnect() {
  while (!client.connected()) {
    if (client.connect(mqtt_client, mqtt_username, mqtt_password)) {
      client.subscribe("@msg/smart");  //subscribe to TOPIC 
    }
    else {
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {

  String message;
  for (int i = 0; i < length; i++) {
    message = message + (char)payload[i];
  }

  //Turn on LED if an incoming message is "open", delay for 2s and then turn it off.
  if (message == "open") {
     myservo.write(35); // สั่งให้ Servo หมุนไปองศาที่ 35
  }
  if(message == "close") {
     myservo.write(160); // สั่งให้ Servo หมุนไปองศาที่ 160
     delay(1000);
     myservo.write(100); // สั่งให้ Servo หมุนไปองศาที่ 100
  }
}


void setup() {
  mySoftwareSerial.begin(9600, SERIAL_8N1, 16, 17);  // speed, type, RX, TX
  Serial.begin(115200);
  pinMode(echo, INPUT); //สั่งให้ขา echo ใช้งานเป็น input
  pinMode(trig, OUTPUT); //สั่งให้ขา trig ใช้งานเป็น output
  myservo.attach(13);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");     
  Serial.println(WiFi.localIP());  
  Serial.println(""); 
  configTime(timezone, dst, ntp_server1, ntp_server2, ntp_server3);
  Serial.println("\nWaiting for time");
  while (!time(nullptr)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  LINE.setToken(LINE_TOKEN);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  while(!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println(F("Unable to begin"));
  }
  Serial.println(F("DFPlayer Mini online."));
  myDFPlayer.volume(20); //ตั้งระดับความดังของเสียง 0-30
}


void loop() {
  
 if (!client.connected()) {
    reconnect();
  }
  client.loop();
 Serial.println(NowTime());
  
 digitalWrite(trig, LOW); 
 delayMicroseconds(5); 
 digitalWrite(trig, HIGH); 
 delayMicroseconds(5); 
 digitalWrite(trig, LOW); //ใช้งานขา trig
 
 duration = pulseIn(echo, HIGH); //อ่านค่าของ echo
 distance = (duration/2) / 29.1; //คำนวณเป็น centimeters
 Serial.print(distance); 
 Serial.print(" cm\n");
  
  if(distance <= 20){ //ระยะการใช้งาน
    myDFPlayer.play(1);
    Serial.println("มีพัสดุมาส่ง");
    LINE.notify("มีพัสดุมาส่ง");
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    root["status"] = "มีพัสดุมาส่ง";
    root["time"] = NowTime();
    String name = Firebase.push("logDevice", root);

    if (Firebase.failed()) {
      Serial.print("pushing /logs failed:");
      Serial.println(Firebase.error());  
      return;
    }
  
    Serial.print("pushed: /logDevice/");
    Serial.println(name);
  }
  else{
         Serial.println("ไม่มีพัสดุมาส่ง");
   }  
  delay(3000); 

}
String NowTime() {
  time_t now = time(nullptr);
  struct tm* p_tm=localtime(&now);
  String timeNow = ""; 
  timeNow += String(p_tm->tm_hour);
  timeNow += ":";
  timeNow += String(p_tm->tm_min);
  timeNow += ":";
  timeNow += String(p_tm->tm_sec);
  timeNow += " ";
  timeNow += String(p_tm->tm_mday); 
  timeNow += "-";
  timeNow += String(p_tm->tm_mon + 1);
  timeNow += "-";  
  timeNow += String(p_tm->tm_year + 1900);
  return timeNow;
}
