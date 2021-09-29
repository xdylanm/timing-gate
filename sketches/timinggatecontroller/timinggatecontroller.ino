//
// Controller for optical timing gate
//
#include <Bounce2.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Fonts/FreeSans12pt7b.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C //
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// optical gate status LED GPIO25
#define OPTICAL_GATE_STATUS_PIN 25

// optical gate status LED GPIO26
#define OPTICAL_GATE_ARMED_PIN 26


// optical gate transitor on GPIO32/ADC4
#define OPTICAL_GATE_SENSE_PIN 32

// mode select push button on GPIO33
#define NEXT_BUTTON_PIN 34

// menu apply push button on GPIO35
#define APPLY_BUTTON_PIN 35

// light sensor threshold
float ambient_light_level = 10.0;
float beam_light_level = 50.0;
float light_threshold = 0.0;

Bounce next_button = Bounce();
Bounce apply_button = Bounce();

hw_timer_t* timer = NULL;
volatile unsigned long lap_time_hundredths = 0;
void IRAM_ATTR on_timer() {
    lap_time_hundredths += 1;
}

float update_light_threshold(float lo, float hi) 
{
    return 0.5*(lo + hi);
}  

void draw_start_screen() 
{
    display.setFont(&FreeSans12pt7b);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);        // Draw white text
    display.setCursor(8,20);                  // Start at top-left corner
    display.println(F("Ready Set.."));
    display.setCursor(8,42);
    display.println(F("..Go!"));
    display.display();
}
    
char const* status_menu[] = {
    "Idle",
    "Ready",
    "Set",
    "Stopwatch",
    "Lap time"};
    
char const* cal_menu[] = {
    "Calibration",
    "Ambient",
    "Beam",
    "Back"};

char const* wifi_menu[] = {
    "Wifi",
    "Enable",
    "Info",
    "Back"};

bool wifi_en = false;
bool wifi_editable = false;
char const* wifi_state_label[] = {"Off","On"};
char const* wifi_edit_label[] = {"Locked","Unlocked"};

void setup() 
{
    pinMode(OPTICAL_GATE_STATUS_PIN, OUTPUT);
    pinMode(OPTICAL_GATE_ARMED_PIN, OUTPUT);

    next_button.attach(NEXT_BUTTON_PIN, INPUT_PULLUP);
    next_button.interval(50);

    apply_button.attach(APPLY_BUTTON_PIN, INPUT_PULLUP);
    apply_button.interval(50);
    
    light_threshold = update_light_threshold(ambient_light_level, beam_light_level);
    
    Serial.begin(115200);
  
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
    }
    Serial.println();
    Serial.println("Acquired display");
  
    draw_start_screen();
    delay(2000);

    display.clearDisplay();
    display.display();
    for (int i = 0; i < 3; ++i) {
        digitalWrite(OPTICAL_GATE_STATUS_PIN, HIGH);
        delay(400);
        digitalWrite(OPTICAL_GATE_STATUS_PIN, LOW);
        delay(400);
    }
}

enum { MENU_STATUS = 0, MENU_CAL, MENU_WIFI } main_menu_index;
enum { STATUS_IDLE = 0, STATUS_READY, STATUS_SET, STATUS_STOPWATCH, STATUS_LAP_TIME } status_display_mode;
enum { CAL_TOP = 0, CAL_AMBIENT, CAL_BEAM, CAL_BACK } cal_menu_index;
enum { WIFI_TOP = 0, WIFI_EN, WIFI_INFO, WIFI_BACK } wifi_menu_index;

char time_string_buf[16];
char sensor_string_buf[16];
char wifi_string_buf[16];

void loop() 
{
    float const raw_val = (4095 - analogRead(OPTICAL_GATE_SENSE_PIN)) / 40.95;  // 0 - 100
    if (raw_val > light_threshold) {  // beam on
        digitalWrite(OPTICAL_GATE_STATUS_PIN, HIGH);
        if (main_menu_index == MENU_STATUS) {
            if (status_display_mode == STATUS_READY) {  // was not armed
                status_display_mode = STATUS_IDLE;
            } else if (status_display_mode == STATUS_SET) { // athlete started
                status_display_mode = STATUS_STOPWATCH;
                digitalWrite(OPTICAL_GATE_ARMED_PIN, LOW);
                timerAlarmEnable(timer);
            }
        }
    } else {  // beam off
        digitalWrite(OPTICAL_GATE_STATUS_PIN, LOW);

        if (main_menu_index == MENU_STATUS) {
            if (status_display_mode == STATUS_STOPWATCH) {  // athlete finishing
                status_display_mode = STATUS_LAP_TIME;
                timerEnd(timer);
                timer = NULL;
            } else if (status_display_mode == STATUS_IDLE) {  // athlete ready, not armed
                status_display_mode = STATUS_READY;    
            }
        }
    }

    next_button.update();
    if (next_button.fell()) {
        if (main_menu_index == MENU_STATUS) {
            if (status_display_mode != STATUS_STOPWATCH) {
                if ((status_display_mode == STATUS_SET) || (status_display_mode == STATUS_LAP_TIME)) {
                    status_display_mode = STATUS_IDLE;
                    digitalWrite(OPTICAL_GATE_ARMED_PIN, LOW);
                }
                main_menu_index = MENU_CAL;
            }
        } else if (main_menu_index == MENU_CAL) {
            if (cal_menu_index == CAL_TOP) {
                main_menu_index = MENU_WIFI;
            } else if (cal_menu_index == CAL_BACK) {
                cal_menu_index = CAL_AMBIENT;
            } else if (cal_menu_index == CAL_AMBIENT) {
                cal_menu_index = CAL_BEAM;
            }else if (cal_menu_index == CAL_BEAM) {
                cal_menu_index = CAL_BACK;
            }
        } else {
            if (wifi_menu_index == WIFI_TOP) {
                main_menu_index = MENU_STATUS;
            } else if (wifi_menu_index == WIFI_BACK) {
                wifi_menu_index = WIFI_EN;
            }  else if (wifi_menu_index == WIFI_EN) {
                wifi_menu_index = WIFI_INFO;
            }  else if (wifi_menu_index == WIFI_INFO) {
                wifi_menu_index = WIFI_BACK;
            }
        }
    }

    apply_button.update();
    if (apply_button.fell()) {
        if (main_menu_index == MENU_STATUS) {
            if (status_display_mode == STATUS_READY) {
                status_display_mode = STATUS_SET;
                digitalWrite(OPTICAL_GATE_ARMED_PIN, HIGH);
                timer = timerBegin(0, 80, true);
                timerAttachInterrupt(timer, &on_timer, true);
                timerAlarmWrite(timer, 10000, true); 
            } else if (status_display_mode == STATUS_SET) {
                status_display_mode = STATUS_READY; 
                digitalWrite(OPTICAL_GATE_ARMED_PIN, LOW);
                timerEnd(timer);
                timer = NULL;
            } else if (status_display_mode == STATUS_LAP_TIME) {
                status_display_mode = STATUS_IDLE; 
                lap_time_hundredths = 0;
            }
        } else if (main_menu_index == MENU_CAL) {
            if (cal_menu_index == CAL_TOP) {
                cal_menu_index = CAL_AMBIENT;
            } else if (cal_menu_index == CAL_AMBIENT) {
                ambient_light_level = (4095 - analogRead(OPTICAL_GATE_SENSE_PIN)) / 40.95;  // 0 - 100
                light_threshold = update_light_threshold(ambient_light_level, beam_light_level);
            } else if (cal_menu_index == CAL_BEAM) {
                beam_light_level = (4095 - analogRead(OPTICAL_GATE_SENSE_PIN)) / 40.95;  // 0 - 100
                light_threshold = update_light_threshold(ambient_light_level, beam_light_level);
            } else if (cal_menu_index == CAL_BACK) {
                cal_menu_index = CAL_TOP;
            }
        } else if (main_menu_index == MENU_WIFI) {
            if (wifi_menu_index == WIFI_TOP) {
                wifi_menu_index = WIFI_EN;
            } else if (wifi_menu_index == WIFI_EN) {
                wifi_en = !wifi_en;
            } else if (wifi_menu_index == WIFI_INFO) {
                wifi_editable = !wifi_editable;
            } else if (wifi_menu_index == WIFI_BACK) {
                wifi_menu_index = WIFI_TOP;
            }
        }
    }

    display.clearDisplay();
    display.setCursor(8,20);                  // Start at top-left corner
    if (main_menu_index == MENU_STATUS) { 
        if (status_display_mode == STATUS_STOPWATCH) {
            unsigned long current_lap_time = lap_time_hundredths;
            unsigned long const mins = lap_time_hundredths / 6000;
            current_lap_time %= 6000;
            unsigned long const secs = current_lap_time / 100;
            current_lap_time %= 100;
            sprintf(time_string_buf, "%02d:%02d.%02d",mins, secs, current_lap_time);
            display.println(time_string_buf);
        } else if (status_display_mode == STATUS_LAP_TIME) {
            unsigned long current_lap_time = lap_time_hundredths;
            unsigned long const mins = lap_time_hundredths / 6000;
            current_lap_time %= 6000;
            unsigned long const secs = current_lap_time / 100;
            current_lap_time %= 100;
            sprintf(time_string_buf, "%02d:%02d.%02d Lap",mins, secs, current_lap_time);
            display.println(time_string_buf);
        } else {
            display.println(status_menu[status_display_mode]);
            delay(10);
        }
    } else if (main_menu_index == MENU_CAL) {
        if (cal_menu_index == CAL_AMBIENT) {
            sprintf(sensor_string_buf, "Amb. %3.1f", raw_val);
            display.println(sensor_string_buf);
            delay(10);
        } else if (cal_menu_index == CAL_BEAM) {
            sprintf(sensor_string_buf, "Beam %3.1f", raw_val);
            display.println(sensor_string_buf);
            delay(10);
        } else {
            display.println(cal_menu[cal_menu_index]);
            delay(10);
        }
    } else if (main_menu_index == MENU_WIFI) {
        if (wifi_menu_index == WIFI_EN) {
            sprintf(wifi_string_buf, "WiFi %s", wifi_state_label[wifi_en]);
            display.println(wifi_string_buf);
            delay(10);
        } else if (wifi_menu_index == WIFI_INFO) {
            // TODO display SSID & password here
            sprintf(wifi_string_buf, "WiFi %s", wifi_edit_label[wifi_editable]);
            display.println(wifi_string_buf);
            delay(10);
        } else {
            display.println(wifi_menu[wifi_menu_index]);
            delay(10);
        }
    }
    display.display();

    
}
