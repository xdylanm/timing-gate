//
// Controller for optical timing gate
//
#include <Bounce2.h>

#include "timinggateserver.h"
#include "displaymanager.h"
#include "menucontroller.h"
#include "stopwatchstate.h"

using MC = MenuController;
auto& mci = MC::get_instance();
    
// optical gate status LED GPIO25
#define OPTICAL_GATE_STATUS_PIN 25

// optical gate status LED GPIO26
#define OPTICAL_GATE_ARMED_PIN 26

// optical gate transitor on GPIO32/ADC4
#define OPTICAL_GATE_SENSE_PIN 32

// mode select push button on GPIO33
#define NEXT_BUTTON_PIN 14

// menu apply push button on GPIO35
#define APPLY_BUTTON_PIN 12

// OLED display
DisplayManager display;

// Server
TimingGateServer web_server(&display);

const char* ssid_ap = "timinggate";
const char* password_ap = "7hag4fom";

const char* ssid = "timinggatetest";
const char* password = "xaMRbWDY";

// light sensor threshold
float ambient_light_level = 10.0;
float beam_light_level = 90.0;
float light_threshold = 0.0;

Bounce next_button = Bounce();
Bounce apply_button = Bounce();

hw_timer_t* timer = NULL;
//volatile unsigned long lap_time_ms = 0;
void IRAM_ATTR on_timer() {
    sw::lap_time_ms += 10;
}

float update_light_threshold(float lo, float hi) 
{
    return 0.5*(lo + hi);
}  

bool wifi_en = false; // to remove

void start_AP();
void on_next();
void on_apply();

void setup() 
{
    sw::beam_state = 0;
    sw::lap_time_ms = 0;
    
    pinMode(OPTICAL_GATE_STATUS_PIN, OUTPUT);
    pinMode(OPTICAL_GATE_ARMED_PIN, OUTPUT);

    next_button.attach(NEXT_BUTTON_PIN, INPUT_PULLUP);
    next_button.interval(50);

    apply_button.attach(APPLY_BUTTON_PIN, INPUT_PULLUP);
    apply_button.interval(50);
    
    light_threshold = update_light_threshold(ambient_light_level, beam_light_level);
    
    Serial.begin(115200);
                      
    if(!display.begin("Laser Gate", "Ready-Set-Go!")) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }
    Serial.println();
    Serial.println("Acquired display");

    for (int i = 0; i < 3; ++i) {
        digitalWrite(OPTICAL_GATE_STATUS_PIN, HIGH);
        digitalWrite(OPTICAL_GATE_ARMED_PIN, LOW);
        delay(400);
        digitalWrite(OPTICAL_GATE_STATUS_PIN, LOW);
        digitalWrite(OPTICAL_GATE_ARMED_PIN, HIGH);
        delay(400);
    }
    digitalWrite(OPTICAL_GATE_ARMED_PIN, LOW);
  
    start_AP();
}


void loop() 
{
    web_server.ws.cleanupClients();
    
    float const raw_val = (4095 - analogRead(OPTICAL_GATE_SENSE_PIN)) / 40.95;  // 0 - 100
    if (raw_val > light_threshold) {  // beam on
        bool do_notify = sw::beam_state != 1;       
        sw::beam_state = 1;
        digitalWrite(OPTICAL_GATE_STATUS_PIN, HIGH);
        if (mci.main_menu_index == MC::MENU_STATUS) {
            if (mci.status_display_mode == MC::STATUS_READY) {  // was not armed
                mci.status_display_mode = MC::STATUS_IDLE;
                do_notify = true;
            } else if (mci.status_display_mode == MC::STATUS_SET) { // athlete started
                mci.status_display_mode = MC::STATUS_STOPWATCH;
                digitalWrite(OPTICAL_GATE_ARMED_PIN, LOW);
                timerAlarmEnable(timer);
                do_notify = true;
            }
            if (do_notify) {
              String const msg = sw::build_message(false);
              web_server.notify_clients(msg);
            }
        }
    } else {  // beam off
        bool do_notify = sw::beam_state != 0;
        sw::beam_state = 0;
        digitalWrite(OPTICAL_GATE_STATUS_PIN, LOW);
        if (mci.main_menu_index == MC::MENU_STATUS) {
            bool sync_clock = false;
            if ((mci.status_display_mode == MC::STATUS_STOPWATCH) && (sw::lap_time_ms > 500)) {  // athlete finishing, debounced
                mci.status_display_mode = MC::STATUS_LAP_TIME;
                timerEnd(timer);
                timer = NULL;
                do_notify = true;
                sync_clock = true;
            } else if (mci.status_display_mode == MC::STATUS_IDLE) {  // athlete ready, not armed
                mci.status_display_mode = MC::STATUS_READY;    
                do_notify = true;
            }
            if (do_notify) {
              String const msg = sw::build_message(sync_clock);
              web_server.notify_clients(msg);
            }
        }
    }

    next_button.update();
    if (next_button.fell()) {
      on_next();
    }

    apply_button.update();
    if (apply_button.fell() || web_server.soft_apply_pressed()) {
      web_server.clear_apply_pressed();
      on_apply();
    }
  
    invalidate_display(raw_val);

}

void start_AP()
{
    display.clear_all();

    // messages
    display.update_title("WiFi Init");
    display.update_status("init AP");
    display.show();
   
    web_server.init_AP(ssid_ap, password_ap);
    
    String status = web_server.wifi_status();
    String current_IP = web_server.active_IP();
    display.update_status(status.c_str(), current_IP.c_str());
    display.show();
    Serial.println(status + " " + current_IP);
    delay(1000);
    display.clear_all();
    display.show();
  
}


void on_next() 
{
    bool sync_mode = false;  
    if (mci.main_menu_index == MC::MENU_STATUS) {
        if (mci.status_display_mode != MC::STATUS_STOPWATCH) {
            if ((mci.status_display_mode == MC::STATUS_SET) || (mci.status_display_mode == MC::STATUS_LAP_TIME)) {
                mci.status_display_mode = MC::STATUS_IDLE;
                digitalWrite(OPTICAL_GATE_ARMED_PIN, LOW);
            }
            mci.main_menu_index = MC::MENU_CAL;
            sync_mode = true;
        }
    } else if (mci.main_menu_index == MC::MENU_CAL) {
        if (mci.cal_menu_index == MC::CAL_TOP) {
            mci.main_menu_index = MC::MENU_WIFI;
            sync_mode = true;
        } else if (mci.cal_menu_index == MC::CAL_BACK) {
            mci.cal_menu_index = MC::CAL_AMBIENT;
        } else if (mci.cal_menu_index == MC::CAL_AMBIENT) {
            mci.cal_menu_index = MC::CAL_BEAM;
        }else if (mci.cal_menu_index == MC::CAL_BEAM) {
            mci.cal_menu_index = MC::CAL_BACK;
        }
    } else {
        if (mci.wifi_menu_index == MC::WIFI_TOP) {
            mci.main_menu_index = MC::MENU_STATUS;
            sync_mode = true;
        } else if (mci.wifi_menu_index == MC::WIFI_BACK) {
            mci.wifi_menu_index = MC::WIFI_EN;
        }  else if (mci.wifi_menu_index == MC::WIFI_EN) {
            mci.wifi_menu_index = MC::WIFI_INFO;
        }  else if (mci.wifi_menu_index == MC::WIFI_INFO) {
            mci.wifi_menu_index = MC::WIFI_BACK;
        }
    }
    if (sync_mode) {
        String const msg = sw::build_message(false);
        web_server.notify_clients(msg);
    }
}

void on_apply() 
{
    if (mci.main_menu_index == MC::MENU_STATUS) {
        if (mci.status_display_mode == MC::STATUS_READY) {
            mci.status_display_mode = MC::STATUS_SET;
            digitalWrite(OPTICAL_GATE_ARMED_PIN, HIGH);
            timer = timerBegin(0, 80, true);
            timerAttachInterrupt(timer, &on_timer, true);
            timerAlarmWrite(timer, 10000, true); 
        } else if (mci.status_display_mode == MC::STATUS_SET) {
            mci.status_display_mode = MC::STATUS_READY; 
            digitalWrite(OPTICAL_GATE_ARMED_PIN, LOW);
            timerEnd(timer);
            timer = NULL;
        } else if (mci.status_display_mode == MC::STATUS_LAP_TIME) {
            mci.status_display_mode = MC::STATUS_IDLE; 
            sw::lap_time_ms = 0;
        }
        String const msg = sw::build_message(false);
        web_server.notify_clients(msg);
    } else if (mci.main_menu_index == MC::MENU_CAL) {
        if (mci.cal_menu_index == MC::CAL_TOP) {
            mci.cal_menu_index = MC::CAL_AMBIENT;
        } else if (mci.cal_menu_index == MC::CAL_AMBIENT) {
            ambient_light_level = (4095 - analogRead(OPTICAL_GATE_SENSE_PIN)) / 40.95;  // 0 - 100
            light_threshold = update_light_threshold(ambient_light_level, beam_light_level);
        } else if (mci.cal_menu_index == MC::CAL_BEAM) {
            beam_light_level = (4095 - analogRead(OPTICAL_GATE_SENSE_PIN)) / 40.95;  // 0 - 100
            light_threshold = update_light_threshold(ambient_light_level, beam_light_level);
        } else if (mci.cal_menu_index == MC::CAL_BACK) {
            mci.cal_menu_index = MC::CAL_TOP;
        }
    } else if (mci.main_menu_index == MC::MENU_WIFI) {
        if (mci.wifi_menu_index == MC::WIFI_TOP) {
            mci.wifi_menu_index = MC::WIFI_EN;
        } else if (mci.wifi_menu_index == MC::WIFI_EN) {
            wifi_en = !wifi_en;
            //web_server.toggle_wifi_en();
        }
        //else if (mci.wifi_menu_index == WIFI_MODE) {
        //    web_server.toggle_wifi_mode();
        //}
        else if (mci.wifi_menu_index == MC::WIFI_BACK) {
            mci.wifi_menu_index = MC::WIFI_TOP;
        }
    }

}

void draw_stopwatch()
{
    char time_string_buf[16];
    if ((mci.status_display_mode == MC::STATUS_STOPWATCH) 
        || (mci.status_display_mode == MC::STATUS_LAP_TIME))
    {
        unsigned long current_lap_time = sw::lap_time_ms;
        unsigned long const mins = sw::lap_time_ms / 60000;
        current_lap_time %= 60000;
        unsigned long const secs = current_lap_time / 1000;
        current_lap_time %= 1000;
        current_lap_time /= 10; // to hundredths
        char const tag = mci.status_display_mode == MC::STATUS_LAP_TIME ? '<' : ' ';
        sprintf(time_string_buf, "%02d:%02d.%02d%c",mins, secs, current_lap_time,tag);
        display.update_vcenter_title(time_string_buf);
    } else {
        display.update_title(MC::status_menu[mci.status_display_mode]);
        delay(10);
    }
}

void draw_cal(float const raw_val) 
{
    char sensor_string_buf[16];
    display.update_title(MC::cal_menu[mci.cal_menu_index]);
    if ((mci.cal_menu_index == MC::CAL_AMBIENT) || (mci.cal_menu_index == MC::CAL_BEAM)) { 
        sprintf(sensor_string_buf, "%3.1f", raw_val);
        display.update_subtitle(sensor_string_buf);
    } 
    delay(10);
}

void draw_wifi()
{
    char wifi_string_buf[16];
    if (mci.wifi_menu_index == MC::WIFI_EN) {
        if (wifi_en /*web_server.wifi_en()*/) {
            display.update_title("WiFi On");
        } else {
            display.update_title("WiFi Off");
        }
    } else if (mci.wifi_menu_index == MC::WIFI_INFO) {
        String ssid_msg = "SSID " + web_server.active_ssid();
        String pswd_msg = "Pass " + web_server.active_pswd();
        display.update_title("WiFi Info");
        display.update_status(
          web_server.wifi_status().c_str(),
          ssid_msg.c_str(),
          pswd_msg.c_str());
    } else {
        display.update_title(MC::wifi_menu[mci.wifi_menu_index]);
    }
    delay(10);
}

void invalidate_display(float const raw_val)
{
    display.clear_all();
    if (mci.main_menu_index == MC::MENU_STATUS) { 
      draw_stopwatch();
    } else if (mci.main_menu_index == MC::MENU_CAL) {
      draw_cal(raw_val);
    } else if (mci.main_menu_index == MC::MENU_WIFI) {
      draw_wifi();
    }
    display.show();
}
