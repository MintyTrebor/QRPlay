#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32QRCodeReader.h>
#include <ezButton.h>

//Change these setting below
#define WIFI_SSID "#####"
#define WIFI_PASSWORD "*******"
//if mister.local does not work replace with IP of your MiSTer eg : http://192.168.1.23:8182/api/games/launch
#define WEBHOOK_URL "http://mister.local:8182/api/games/launch"
#define RESET_URL "http://mister.local:8182/api/launch/menu"
//change to false if you do not want the MiSTer to reset to the menu on removal of game card
#define RESET_ON_REMOVE true

//Set the GPIO Pin of the micro switch
#define BUTTON_PIN 32

//Change this if you use a different ESP32 Cam Model.
//The QR code Library currently supports:
//CAMERA_MODEL_ESP_EYE
//CAMERA_MODEL_WROVER_KIT
//CAMERA_MODEL_M5STACK_PSRAM
//CAMERA_MODEL_M5STACK_V2_PSRAM
//CAMERA_MODEL_M5STACK_WIDE
//CAMERA_MODEL_M5STACK_ESP32CAM
//CAMERA_MODEL_AI_THINKER
//CAMERA_MODEL_TTGO_T_JOURNAL
ESP32QRCodeReader reader(CAMERA_MODEL_WROVER_KIT);


struct QRCodeData qrCodeData;
struct QRCodeData EmptyQRCodeData;
bool isConnected = false;
ezButton limitSwitch(BUTTON_PIN);


bool connectWifi()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    return true;
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int maxRetries = 10;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    maxRetries--;
    if (maxRetries <= 0)
    {
      return false;
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  return true;
}

void callWebhook(String code)
{
  HTTPClient http;
  WiFiClient client;
  http.begin(client, WEBHOOK_URL);                   
  //Specify content-type header
  http.addHeader("Content-Type", "application/json");
  //post the QR data (should json {"path": "xxx"} str)
  int httpResponseCode = http.POST(code);
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);            
  http.end();
}

void callResetMister()
{
  HTTPClient http;
  WiFiClient client;
  http.begin(client, RESET_URL);                   
  int httpResponseCode = http.POST("{}");
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);            
  http.end();
}

void readQRCode()
{
  qrCodeData = EmptyQRCodeData;
  reader.begin();
  Serial.println("Begin QR Code reader");
  //set camera to greyscale for better detection
  sensor_t * s = esp_camera_sensor_get();
  s->set_special_effect(s, 2);
  bool waitForQR = true;
  while (waitForQR)
  {
    if (reader.receiveQrCode(&qrCodeData, 100))
    {
      Serial.println("Found QRCode");
      if (qrCodeData.valid)
      {
        Serial.print("Payload: ");
        Serial.println((const char *)qrCodeData.payload);
        callWebhook(String((const char *)qrCodeData.payload));
        waitForQR = false;
      }
      else
      {
        Serial.print("Invalid: ");
        Serial.println((const char *)qrCodeData.payload);
      }
    }
  }
  reader.end();
  Serial.println("End QR Code reader");

}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  limitSwitch.setDebounceTime(50);
  reader.setup();
  Serial.println("Setup QRCode Reader");
  Serial.println("Waiting for Trigger");
}

void loop()
{ 
  bool connected = connectWifi();
  if (isConnected != connected)
  {
    isConnected = connected;
  }
  if (isConnected)
  {
    limitSwitch.loop(); 

    if(limitSwitch.isPressed()){
      Serial.println("Game Card Inserted");
      readQRCode();
    }

    if(limitSwitch.isReleased()){
      Serial.println("Game Card Removed");
      if (RESET_ON_REMOVE){
        Serial.println("Resetting Mister Now!");
        callResetMister();
      }
      ESP.restart();
    }

    int state = limitSwitch.getState();

  }
  delay(300);
}