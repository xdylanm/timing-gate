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
    "Wifi",
    "Enable",
    "Mode",
    "Info",
    "Back"};

MenuController::MenuController()
{

}

MenuController& MenuController::get_instance()
{
    if (!instance_) {
      instance_ = new MenuController();
    }

    return *instance_;
  
}
