#ifndef _menucontroller_h
#define _menucontroller_h

#include <Arduino.h>

class MenuController
{
public:
    MenuController(MenuController const& ) = delete;
    void operator=(MenuController const& ) = delete;

    static MenuController& get_instance();

    enum { MENU_STATUS = 0, MENU_CAL, MENU_WIFI } main_menu_index;
    enum { STATUS_IDLE = 0, STATUS_READY, STATUS_SET, STATUS_STOPWATCH, STATUS_LAP_TIME } status_display_mode;
    enum { CAL_TOP = 0, CAL_AMBIENT, CAL_BEAM, CAL_BACK } cal_menu_index;
    enum { WIFI_TOP = 0, WIFI_INFO, WIFI_MODE, WIFI_BACK } wifi_menu_index;
    enum WiFiInfoMenuChoice { WIFI_INFO_TOP = 0, WIFI_INFO_ADDR, WIFI_INFO_SSID, WIFI_INFO_PASS, WIFI_INFO_BACK};
    enum WiFiModeMenuChoice { WIFI_MODE_TOP = 0, WIFI_MODE_AP, WIFI_MODE_STA, WIFI_MODE_OFF, WIFI_MODE_BACK};

    WiFiInfoMenuChoice wifi_info_menu_index;
    WiFiModeMenuChoice wifi_mode_menu_index;

    static char const* status_menu[];
    static char const* cal_menu[];
    static char const* wifi_menu[];
    static char const* wifi_info_menu[];
    static char const* wifi_mode_menu[];

private:
    MenuController();
    static MenuController* instance_;

};

#endif //_menucontroller_h
