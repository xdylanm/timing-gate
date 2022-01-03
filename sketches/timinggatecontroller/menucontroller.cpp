#include "menucontroller.h"
#include "stopwatchstate.h"

MenuController* MenuController::instance_ = nullptr;

char const* MenuController::status_menu[] = {
    "Idle",
    "Ready",
    "Set",
    "Stopwatch",
    "Lap time"};

char const* MenuController::cal_menu[] = {
    "Calibration",
    "Ambient",
    "Beam",
    "Back"};

char const* MenuController::wifi_menu[] = {
    "WiFi",
    "Info",
    "Mode",
    "Back"};

char const* MenuController::wifi_info_menu[] = {
    "Info",
    "WiFi Addr",
    "WiFi SSID",
    "WiFi Passwd",
    "Back"};

char const* MenuController::wifi_mode_menu[] = {
    "Mode",
    "Set AP",
    "Set STA",
    "Disable",
    "Back"};

MenuController::MenuController()
  : main_menu_index(MENU_STATUS), status_display_mode(STATUS_IDLE), cal_menu_index(CAL_TOP),
    wifi_menu_index(WIFI_TOP), wifi_info_menu_index(WIFI_INFO_TOP), wifi_mode_menu_index(WIFI_MODE_TOP)
{

}

MenuController& MenuController::get_instance()
{
    if (!instance_) {
      instance_ = new MenuController();
    }

    return *instance_;
  
}
