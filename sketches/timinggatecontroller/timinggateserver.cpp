#include "timinggateserver.h"
#include "htmltemplate.h"

#include <WiFiAP.h>

 
AsyncWebServer TimingGateServer::server(80);
AsyncWebSocket TimingGateServer::ws("/ws");
bool TimingGateServer::apply_msg = false;

TimingGateServer::TimingGateServer(DisplayManager* display /*=nullptr*/)
: disp(display), enabled(false)
{

}

bool TimingGateServer::disconnect() 
{
    if (!enabled) {
      return true;
    }
    bool disconn_ok = true;
    if (WiFi.getMode() & WIFI_MODE_STA) {
        disp_msg("disconnecting STA", "Disconnecting station");
        disconn_ok = WiFi.disconnect(true);
    } else if (WiFi.getMode() & WIFI_MODE_AP) {
        disp_msg("disconnecting AP", "Disconnecting AP");
        disconn_ok = WiFi.softAPdisconnect(true);
    }
    if (!disconn_ok) {
        disp_msg("failed", "Failed to disconnect");
        delay(1000);
        return false;
    }
    WiFi.mode(WIFI_OFF);
    enabled = false;
    return true;
}

bool TimingGateServer::init_STA(const char* ssid, const char* password)
{
    if (!disconnect()) {
      return false;
    }
    // Connect to Wi-Fi 
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        disp_msg("connecting", "Connecting to WiFi..");
    }
  
    current_ssid = String(ssid);
    current_pswd = String(password);

    // Print ESP Local IP Address
    String long_msg = "WiFi connected to " + WiFi.localIP();
    disp_msg("connected", long_msg.c_str());
    delay(500);

    init_server();
    return true;
}

bool TimingGateServer::init_AP(const char* ssid, const char* password) 
{
    WiFi.softAP(ssid, password);

    //IPAddress apIP = WiFi.softAPIP();
    //Serial.print("AP address: ");
    //Serial.println(apIP);
    delay(1000);

    /*
    disp_msg("init AP", "Initalizing Access Point (AP)");
    delay(500);
    disp_msg(ssid,ssid);
    delay(500);
    disp_msg(password,password);
    delay(500);
    //if (!disconnect()) {
    //  return false;
    //}
    
    String long_msg = "Starting Access Point (AP) at 192.168.1.1" + String(", SSID: ") 
        + String(ssid) + String(", passwd: ") + String(password);
    disp_msg("starting", long_msg.c_str());
    delay(1000);
    
    //WiFi.enableAP(true);
    //delay(200);
    //IPAddress apIP(192,168,1,1);
    //IPAddress netmask(255,255,255,0);
    //WiFi.softAPConfig(apIP, apIP, netmask);
    WiFi.softAP(ssid, password);
    delay(2000);
    
    current_ssid = String(ssid);
    current_pswd = String(password);

    // Print ESP Local IP Address
    long_msg = "AP address: " + WiFi.softAPIP();
    disp_msg("AP ready", long_msg.c_str());
    delay(1000);

    */

    current_ssid = String(ssid);
    current_pswd = String(password);
    init_server();
    return true;
}
  
void TimingGateServer::init_server() 
{
    initWebSocket();

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", HTML::index);
    });

    // Start server
    server.begin();
    disp_msg("Server OK", "Started server");
    delay(1000);
}

String TimingGateServer::wifi_status() const
{
    switch (WiFi.getMode()) {
    case WIFI_MODE_AP:
        return String("AP ") + WiFi.softAPIP().toString();
    case WIFI_MODE_STA:
        return String("STA ") + WiFi.localIP().toString();
    default:
        return String("Disconnected");
    }
}

String TimingGateServer::active_ssid() const
{
  return current_ssid;
}

String TimingGateServer::active_pswd() const
{
  return current_pswd;
}


void TimingGateServer::disp_msg(const char* msg, const char* long_msg) 
{
    if (long_msg) {
        Serial.println(long_msg);
    }
  
    if (!disp) {
        return;
    }
    disp->update_status(msg);
    disp->show();
}

// broadcast to all connected clients
void TimingGateServer::notifyClients(String const& msg) {
    if (enabled) {
      ws.textAll(msg);
    }
}

void TimingGateServer::initWebSocket() 
{
    if (enabled) {
        ws.onEvent(TimingGateServer::onEvent);
        server.addHandler(&ws);
    }
}

// client sent a message: apply button pressed
void TimingGateServer::handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "apply") == 0) {
      apply_msg = true;
    }
  }
}
  
void TimingGateServer::onEvent(
    AsyncWebSocket *server, 
    AsyncWebSocketClient *client, 
    AwsEventType type,
    void *arg, 
    uint8_t *data, 
    size_t len) 
{
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      //ws.text(client->id(),buildMessage(true)); 
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}
