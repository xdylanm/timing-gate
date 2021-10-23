#include "timinggateserver.h"
#include "htmltemplate.h"
#include "stopwatchstate.h"

#include <WiFiAP.h>

 
AsyncWebServer TimingGateServer::server(80);
AsyncWebSocket TimingGateServer::ws("/ws");
bool TimingGateServer::received_apply_msg = false;

TimingGateServer::TimingGateServer(DisplayManager* display /*=nullptr*/)
: disp_(display), enabled_(false)
{

}

bool TimingGateServer::disconnect() 
{
    if (!enabled_) {
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
    enabled_ = false;
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

    enabled_ = true;
    current_ssid_ = String(ssid);
    current_pswd_ = String(password);

    // Print ESP Local IP Address
    String long_msg = "WiFi connected to " + WiFi.localIP();
    disp_msg("connected", long_msg.c_str());
    delay(500);

    init_server();
    return true;
}

bool TimingGateServer::init_AP(const char* ssid, const char* password) 
{

    WiFi.enableAP(true);
    delay(200);
    IPAddress apIP(192,168,1,1);
    IPAddress netmask(255,255,255,0);
    WiFi.softAPConfig(apIP, apIP, netmask);

    WiFi.softAP(ssid, password);
    delay(1000);

    enabled_ = true;
    
    current_ssid_ = String(ssid);
    current_pswd_ = String(password);
    
    init_server();
    
    return true;
}
  
void TimingGateServer::init_server() 
{
    ws.onEvent(TimingGateServer::on_event);
    server.addHandler(&ws);

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
        return String("AP");
    case WIFI_MODE_STA:
        return String("STA");
    default:
        return String("Disconnected");
    }
}

String TimingGateServer::active_IP() const
{
    switch (WiFi.getMode()) {
    case WIFI_MODE_AP:
        return WiFi.softAPIP().toString();
    case WIFI_MODE_STA:
        return WiFi.localIP().toString();
    default:
        return String();
    }
}

String TimingGateServer::active_ssid() const
{
  return current_ssid_;
}

String TimingGateServer::active_pswd() const
{
  return current_pswd_;
}


void TimingGateServer::disp_msg(const char* msg, const char* long_msg) 
{
    if (long_msg) {
        Serial.println(long_msg);
    }
  
    if (!disp_) {
        return;
    }
    disp_->update_status(msg);
    disp_->show();
}

// broadcast to all connected clients
void TimingGateServer::notify_clients(String const& msg) {
    if (enabled_) {
        ws.textAll(msg);
    }
}

// client sent a message: apply button pressed
void TimingGateServer::handle_web_socket_message(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "apply") == 0) {
      received_apply_msg = true;
    }
  }
}

void TimingGateServer::on_event(
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
      ws.text(client->id(),sw::build_message(true)); 
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      Serial.printf("WebSocket received data len %d from client #%u",len, client->id());
      handle_web_socket_message(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}
