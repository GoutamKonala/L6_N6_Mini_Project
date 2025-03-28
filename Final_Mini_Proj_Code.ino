#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "****************"
#define WIFI_PASSWORD "******************"
#define DATABASE_URL "https://****************.asia-southeast1.firebasedatabase.app/"
#define API_KEY "**************************************"
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SOUND_SPEED 0.034
#define BIN_HEIGHT 30

FirebaseData fbdo; 
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int trigPin = 32;
const int echoPin = 33;
const int mqPin = 36;
const int servoPin = 13;
const int pirPin = 26;

int pos = 0;
int pirstatus;
long duration;
float distance;
int gaspressure;
int fill;
int organics;
String status;

Servo myservo;

void setup()
{

  Serial.begin(115200);
  Serial.println("Hello!");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);
  }
  Serial.println();
  Serial.print("Connected with IP : ");
  Serial.println(WiFi.localIP());
  Serial.println();

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  if(Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("Signup OK");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(mqPin, INPUT);
  pinMode(pirPin,INPUT);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);
  myservo.attach(servoPin, 500, 2400);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) 
  { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); 
  }
  display.display();
  delay(2000); 
  display.clearDisplay();
  display.setTextColor(WHITE);
}

void loop()
{

  pir();

  firebase();

  //display_smartbin**********
  display.clearDisplay();
  display.setTextSize(3);
  display.setCursor(10, 10);
  display.println("SMART");
  display.setCursor(20, 35);
  display.println("BIN");
  display.display();
  delay(1000);
  //display_smartbin**********
}

void pir()
{
  //pir_servo**********
  pirstatus = digitalRead(pirPin);
  if(pirstatus == HIGH)
  {
    //open**********
    for (pos = 0; pos <= 180; pos += 1) 
    { 
      myservo.write(pos); 
      delay(15);          
    }
    //open**********

    //pir_firebase_open
    if(Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0))
    {
      status = "opened";
      sendDataPrevMillis == millis();
      if(Firebase.RTDB.setString(&fbdo, "test/status", status))
      {
        Serial.println();
        Serial.println(status);
        Serial.println("Successfully saved to : " + fbdo.dataPath());
        Serial.println(" (" + fbdo.dataType() + ")");
      }
      else
      {
        Serial.println("FAILED : " + fbdo.errorReason());
      }
    }//pir_firebase_open

    //display_open**********
    display.clearDisplay();
    display.setCursor(10, 10);
    display.print("opened");
    display.display();
    //display_open**********

    delay(6000);
    
    //close**********
    for (pos = 180; pos >= 0; pos -= 1) 
    { 
      myservo.write(pos); 
      delay(15);          
    }
    //close**********

    //pir_firebase_close
    if(Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0))
    {
      status = "closed";
      sendDataPrevMillis == millis();
      if(Firebase.RTDB.setString(&fbdo, "test/status", status))
      {
        Serial.println();
        Serial.println(status);
        Serial.println("Successfully saved to : " + fbdo.dataPath());
        Serial.println(" (" + fbdo.dataType() + ")");
      }
      else
      {
        Serial.println("FAILED : " + fbdo.errorReason());
      }
    }
    //pir_firebase_close

    //display_close**********
    display.clearDisplay();
    display.setCursor(10, 10);
    display.print("closed");
    display.display();
    //display_close**********
  }
  //pir_servo**********
}

void firebase()
{
  //firebase**********
  if(Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis == millis();
    ultrasonic();
    mq4();
    if(Firebase.RTDB.setInt(&fbdo, "test/fill", fill))
    {
      Serial.println();
      Serial.println(fill);
      Serial.println("Successfully saved to : " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    }
    else
    {
      Serial.println("FAILED : " + fbdo.errorReason());
    }

    if(Firebase.RTDB.setInt(&fbdo, "test/organics", organics))
    {
      Serial.println();
      Serial.println(organics);
      Serial.println("Successfully saved to : " + fbdo.dataPath());
      Serial.println(" (" + fbdo.dataType() + ")");
    }
    else
    {
      Serial.println("FAILED : " + fbdo.errorReason());
    }
  }
  //firebase**********
}
void ultrasonic()
{
  //ultrasonic**********
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * SOUND_SPEED/2;
  fill = 100 - (distance * 100/BIN_HEIGHT);
  Serial.println();
  Serial.print("fill: ");
  Serial.print(fill);
  Serial.println("%");
  delay(100);
  //ultrasonic**********

  //display_ultrasonic**********
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 10);
  display.println("Fill:");
  display.setCursor(20, 30);
  display.print(fill);
  display.println("%");
  display.display();
  delay(1000);
  //display_ultrasonic**********
}
void mq4()
{
  //mq**********
  gaspressure = analogRead(mqPin);
  organics = gaspressure * 100/2250;
  Serial.println();
  Serial.print("organics:");
  Serial.print(organics);
  Serial.println("%");
  //mq**********

  //display_mq**********
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 10);
  display.println("Organics:");
  display.setCursor(20, 30);
  display.print(organics);
  display.println("%");
  display.display();
  delay(1000);
  //display_mq**********
}