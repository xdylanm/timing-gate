#include "stopwatchstate.h"
#include "menucontroller.h"

namespace sw 
{

extern volatile unsigned long lap_time_ms = 0;
extern volatile int beam_state = 0;

String build_message(bool sync_time)
{
  using MC = MenuController;
  auto& mci = MC::get_instance();
  String msg = "{";
  if (sync_time) {
    msg += "\"sync_elapsed_ms\":";
    msg += lap_time_ms;
    msg += ",";
  }
  msg += "\"beam\":";
  msg += beam_state;
  if (mci.main_menu_index == MC::MENU_STATUS) {
    msg += ",\"state\":";
    if (mci.status_display_mode == MC::STATUS_IDLE) {
      msg += "\"Idle\"";
    } else if (mci.status_display_mode == MC::STATUS_READY) {
      msg += "\"Ready\"";
    } else if (mci.status_display_mode == MC::STATUS_SET) {
      msg += "\"Set\"";
    } else if (mci.status_display_mode == MC::STATUS_STOPWATCH) {
      msg += "\"Go\"";
    } else if (mci.status_display_mode == MC::STATUS_LAP_TIME) {
      msg += "\"Finished\"";
    } else {
      msg += "\"Undefined\""; // error
    }
  }
  msg += ",\"mode\":";
  if (mci.main_menu_index == MC::MENU_STATUS) {
    msg += "\"Stopwatch\"";
  } else if (mci.main_menu_index == MC::MENU_CAL) {
    msg += "\"Calibration\"";
  } else if (mci.main_menu_index == MC::MENU_WIFI) {
    msg += "\"Wifi\"";
  } else {
    msg += "\"Undefined\""; // error
  }
  msg += "}";
  Serial.println(msg);
  return msg;

}


} // namespace sw
