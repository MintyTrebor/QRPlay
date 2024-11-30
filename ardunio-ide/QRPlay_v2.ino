#include <Arduino.h>
#include <WiFi.h>
#include <ESP32CameraPins.h>
#include <ESP32QRCodeReader.h>
#include <ezButton.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <ArduinoWebsockets.h>
#include <UUID.h>
#include <atomic>
#include "index.h"
#include "qr_code_js.h"

//Wifi Info
#define WIFI_SSID "#####"
#define WIFI_PASSWORD "******"

//Set the GPIO Pin of the micro switch
#define BUTTON_PIN 32

//different ESP32 Cam Models supported by QR library. Note Only WROVER board is tested for this project
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

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
using namespace websockets;
Preferences preferences;

struct QRCodeData qrCodeData;
struct QRCodeData EmptyQRCodeData;
bool isConnected = false;
bool lastCmdOK = false;
bool isWebLog = false;
String ZAP_URL = "ws://<replace>:7497";
ezButton limitSwitch(BUTTON_PIN);

void notifyClients(String txtMsgToSend) {
  Serial.println(txtMsgToSend);
  if(isWebLog){
    ws.textAll(txtMsgToSend);
    if(txtMsgToSend == "closeWS"){
      delay(500);
      ESP.restart();
    }
  }else{
    if(txtMsgToSend == "closeWS"){
      ESP.restart();
    }
  }  
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "Reset") == 0) {
      isWebLog = true;
      notifyClients("Resetting QRPlay - Remove any Game Card");
      isWebLog = false;
      notifyClients("closeWS");
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      isWebLog = true;
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      isWebLog = false;
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}


int ZaparooSendCmd(JsonDocument payload){  
  String ZapIP = preferences.getString("misterIP", "mister.local");
  String newURL = ZAP_URL;
  std::atomic<bool> complete;
  std::atomic<int> lastError;
  newURL.replace("<replace>", ZapIP);
  String wsTmpStr = "ZAP URL: " + newURL;
  notifyClients(wsTmpStr);
  WebsocketsClient client;
  lastError.store(0);
  complete.store(false);
  UUID uuid;
  const char* id = uuid.toCharArray();
  payload["id"]= uuid.toCharArray();
  client.onMessage([&, &id](WebsocketsMessage msg){
    if(complete.load()) return;
    JsonDocument result;
    DeserializationError error = deserializeJson(result, msg.data());
    if (error) {
      lastError.store(4); //Failed to parse json
      complete.store(true);
      return;
    }
    const char* resultId = result["id"];
    if(strcmp(id, resultId) != 0) return;
    if(result.containsKey("result")){
      lastError.store(3); //Failed to launch path
    }
    complete.store(true);
  });
  if(!client.connect(newURL)){
    return 2;
  }
  String request;
  serializeJson(payload, request);
  client.send(request);
  while(!complete.load()){
    client.poll();
  }
  client.close();
  return lastError.load();
}


bool connectWifi()
{
  if (WiFi.status() == WL_CONNECTED){
    return true;
  }
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int maxRetries = 10;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    maxRetries--;
    if (maxRetries <= 0)    {
      return false;
    }
  }
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
  WiFi.setSleep(false);
  server.begin();
  initWebSocket();
  return true;
}

bool launchGame(String code)
{
  String newCode = code;
  newCode.replace("{\"path\":\"", "");
  newCode.replace("\"}", "");
  String wsTmpStr = "Zaparoo Lauch Parameter: " + newCode;
  notifyClients(wsTmpStr);
  JsonDocument ZapCmd;
  ZapCmd["jsonrpc"]= "2.0";
  ZapCmd["method"]="launch";
  ZapCmd["id"]= "";
  ZapCmd["params"]["text"] = newCode;
  ZapCmd["params"]["uid"] = "52f6242e-7a5a-11ef-bf93-020304050607"; //fake
  ZapCmd.shrinkToFit();
  int resCode = ZaparooSendCmd(ZapCmd);
  bool launched = resCode == 0;
  if(launched){
    notifyClients("Launched Game");
    return true;
  }else{
    wsTmpStr = "Zaparoo Error Launching Game - code: " + resCode;
    notifyClients(wsTmpStr);
    return false;
  }
}

void callResetMister()
{
  JsonDocument ZapCmd;
  ZapCmd["jsonrpc"]= "2.0";
  ZapCmd["method"]="stop";
  ZapCmd["id"]= "";
  ZapCmd.shrinkToFit();
  int resCode = ZaparooSendCmd(ZapCmd);
}

String readQRCode()
{
  qrCodeData = EmptyQRCodeData;
  reader.begin();
  notifyClients("Started QRCode Detection");
  //set camera to greyscale for better detection
  sensor_t * s = esp_camera_sensor_get();
  s->set_special_effect(s, 2);
  bool waitForQR = true;
  int maxRetries = 10;
  while (waitForQR)
  {
    if (reader.receiveQrCode(&qrCodeData, 100))
    {
      notifyClients("Found QRCode");
      if (qrCodeData.valid)
      {
        waitForQR = false;
        return (const char *)qrCodeData.payload;
      }
      else
      {
        
        maxRetries--;
        if (maxRetries <= 0){
          waitForQR = false;
          notifyClients("Max Retries Reached - Aborting");
          reader.end();
          return "ERROR";
        }else{
          notifyClients("QR Code reading invalid - Retrying");
        }
      }
    }
  }
  notifyClients("QR Code Detection Stopped");
}

void cardInserted()
{
  String tmpQRCode = readQRCode();
  String wsTmpStr = "QR Code Raw Payload : " + tmpQRCode;
  notifyClients(wsTmpStr);
  if(tmpQRCode != "ERROR"){
    bool LaunchedGameOK = launchGame(tmpQRCode);
    if(!LaunchedGameOK){
      notifyClients("MiSTer failed to launch game. Check MiSTer is powered and the MiSTer IP is correct, then fullyremove and re-insert game card");
    }
  }else{
    notifyClients("Error Reading QR Code - Please remove and insert a valid Game Card. QRPlay will reset on card removal");
  }
}

void setup()
{
  Serial.begin(115200);
  limitSwitch.setDebounceTime(50);
  preferences.begin("qrplay", false);
  bool connected = connectWifi();
  reader.setup();
  notifyClients("QRPlay Ready");
  String wsTmpStr = "";
  // Initialize WebPages  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = HTML_CONTENT_HOME;
    if (request->hasArg("misterIP")) {
      String newMisterIP = request->arg("misterIP");
      preferences.putString("misterIP", newMisterIP);
      String wsTmpStr = "MiSTer IP Set to: " + newMisterIP;
      notifyClients(wsTmpStr);
    }
    if (request->hasArg("selResetOnRem")) {
      String newSelResetOnRem = request->arg("selResetOnRem");
      bool newBreset = true;
      if(newSelResetOnRem == "false"){
        newBreset = false;  
      }
      preferences.putBool("Reset_On_Remove", newBreset);
      String wsTmpStr = "Reset On Remove Option Set to: " + newSelResetOnRem;
      notifyClients(wsTmpStr);
    }
    if(!preferences.getBool("Reset_On_Remove", true)){
      html.replace("<option value=\"false\">No</option>", "<option value=\"false\" selected>No</option>");
    }
    String tmpMIP = preferences.getString("misterIP", "mister.local");
    html.replace("mister.local", tmpMIP);
    request->send(200, "text/html", html);
  });
  server.on("/qrcode.js", HTTP_GET, [](AsyncWebServerRequest *request){
    String html = QRCODE_JS;
    request->send(200, "text/plain", html);
  });
}

void loop()
{ 
  bool connected = connectWifi();
  if (isConnected != connected){
    isConnected = connected;
  }
  if (isConnected) {
    limitSwitch.loop(); 
    if(limitSwitch.isPressed()){
      notifyClients("Game Card Inserted");
      cardInserted();
    }
    if(limitSwitch.isReleased()){
      notifyClients("Game Card Removed");
      if (preferences.getBool("Reset_On_Remove", true)){
        notifyClients("Resetting Mister!");
        callResetMister();
      }
      notifyClients("Resetting QR Play Now!");
      notifyClients("closeWS");
    }
    int state = limitSwitch.getState();
  }
  delay(300);
}