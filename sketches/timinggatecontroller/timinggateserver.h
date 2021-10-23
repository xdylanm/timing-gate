#ifndef _timinggateserver_h
#define _timinggateserver_h

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_SSD1306.h>
#include "displaymanager.h"

class TimingGateServer 
{
public:
  TimingGateServer(DisplayManager* display = nullptr);
  
  bool disconnect();
  bool init_STA(const char* ssid, const char* password);
  bool init_AP(const char* ssid, const char* password);

  void notify_clients(String const& msg);

  bool soft_apply_pressed() const { return received_apply_msg; }
  bool clear_apply_pressed() { received_apply_msg = false; }

  String wifi_status() const;
  String active_IP() const;
  String active_ssid() const;
  String active_pswd() const;

  static AsyncWebServer server;
  static AsyncWebSocket ws;

private:
  void init_server();

  static void on_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len);
  static void handle_web_socket_message(void *arg, uint8_t *data, size_t len);

  static bool received_apply_msg;
  void disp_msg(const char* msg, const char* long_msg);

  bool enabled_;
  DisplayManager* disp_;

  String current_ssid_;
  String current_pswd_;

};

#endif // _timinggateserver_h
