//
// Controller for optical timing gate
//

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

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

const char* ssid = <SSID>;
const char* password = <password>;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html>
<head>
    <title>Laser Timing Gate</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      html {
        min-height: 100%;
        background-image: radial-gradient(white, #c0c0c0);
      }
      
      #overlay {
        position: fixed; /* Sit on top of the page content */
        display: none; /* Hidden by default */
        width: 100%; /* Full width (cover the whole page) */
        height: 100%; /* Full height (cover the whole page) */
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: rgba(0,0,0,0.5); /* Black background with opacity */
        z-index: 2; /* Specify a stack order in case you're using a different order for other elements */
      }
      
      .center {
        position: fixed;
        top: 20%;
        left: 10%;
        width: 80%;
        /*background-color: gray;*/
      
        font-family: 'Lucida Console', 'Courier New', Courier, monospace;
      }
      
      .swtime {
        position: relative;
        width: 100%;
        /*background-color: wheat;*/
        font-size: 14vw;
        text-align: center;
      }
      
      .swheader {
        position: relative;
        left: 8%;
        width: 84%;  
        /*background-color: lightblue;*/
      }
      
      .swstatus {
        /*background-color: lightgreen;*/
        font-size: 6vw;
        padding-top: 1vw;
        text-align: left;
        float: left;
      }
      
      .swcomponent {
        display: inline-block;
      }
      
      #beamState {
        font-size: 6vw;
        padding-top: 2vw;
        background-color: #ff8080;
        color: transparent;
        float: right;
        margin-left: 1vw;
      }
      
      .swcontrol {
        position: relative;
        float: right;
      }
      
      .btn {
        border: 2px solid black;
        background-color: white;
        color: black;
        padding-top: 1vw;
        padding-bottom: 1vw;
        font-family: 'Lucida Console', 'Courier New', Courier, monospace;
      }
      
      
      .nextbtn {
        width: 24vw;
        font-size: 6vw;
      }
      
      .nextbtn:hover {
        background-color: #d8d8d8;
      }
      
      .nextbtn:disabled {
        background-color: white;
        border: 2px solid #707070;
        color: #707070;
      }
    </style>
    <script>
      // stopwatch
      const sw = {
          lblMin: null,
          lblSec: null,
          lblHun: null,
      
          started: false,
          running: false,
      
          tStart: 0,
          intervalId: 0,
      
          init: function () {
              sw.lblMin = document.getElementById("MM");
              sw.lblSec = document.getElementById("SS");
              sw.lblHun = document.getElementById("NN");
              sw.started = false;
              sw.resetTimer();
          },
      
          startTimer: function () {
              if (!sw.started) {
                  sw.started = true;
                  sw.stop();
                  sw.start();
              }
          },
          stopTimer: function () {
              if (sw.started) {
                  sw.started = sw.running = false;
                  sw.stop();
              }
          },
      
          resetTimer: function () {
              if (!sw.started) {
                  sw.running = false;
                  sw.lblMin.textContent = '00';
                  sw.lblSec.textContent = '00';
                  sw.lblHun.textContent = '00';
              }
          },
      
          externalSyncElapsedTime: function(elapsed) {
              // adjust the start time to sync elapsed time
              // example: locally, 12s elapsed; external, 10s --> local clock is fast
              //          --> advance the local start time 12-10 = 2s
              const localElapsed = performance.now() - sw.tStart;
              const diffElapsed = localElapsed - elapsed;
              sw.tStart += diffElapsed;
              sw.timerCycle();
          },
      
          timerCycle: function () {
              if (sw.running) {
                  const t = performance.now();  // millis
                  sw.updateFields(t - sw.tStart);
              }
          },
      
          updateFields: function(elapsed) {
              const elapsedSec = elapsed*0.001;
              const tt = Math.floor(elapsedSec);
              const mil = elapsedSec - tt;
              sw.lblHun.textContent = (mil).toFixed(2).slice(-2);
              const sec = tt % 60;
              //if (sec == ps) return;
              //ps = sec;
              const min = Math.floor(tt / 60) % 60;
              sw.lblMin.textContent = ('0' + min).slice(-2);
              sw.lblSec.textContent = ('0' + sec).slice(-2);
          },
      
          stop: function () {
              if (sw.intervalId) {
                  clearInterval(sw.intervalId);
                  sw.intervalId = 0;
              }
          },
      
          start: function () {
              if (sw.started && !sw.running) {
                  sw.running = true;
                  sw.tStart = performance.now();
                  sw.intervalId = setInterval(sw.timerCycle, 31);
              }
          }
      };
      
      
      
      // state machine
      const sm = {
          btn: null,
          lbl: null,
          beam: null,
      
          beamState: false,
          statusState: "",
      
          gateActive: false,
      
          websocket: null,
          
          initWebSocket: function () {
              console.log('Trying to open a WebSocket connection...');
              const gateway = `ws://${window.location.hostname}/ws`;
              websocket = new WebSocket(gateway);
              websocket.onopen = sm.onOpen;
              websocket.onclose = sm.onClose;
              websocket.onmessage = sm.onMessage; // <-- add this line
          },
      
          onOpen: function (event) {
              console.log('Connection opened');
          },
      
          onClose: function (event) {
              console.log('Connection closed');
              sm.toInactive();
              setTimeout(sm.initWebSocket, 2000);
          },
      
          onMessage: function (event) {
              console.log(event.data);
              const obj = JSON.parse(event.data);

              // first update the clock (jitter)
              if (obj.sync_elapsed_ms != undefined) {
                  sw.externalSyncElapsedTime(obj.sync_elapsed_ms);
              }
              // then update the beam state
              if (obj.beam != undefined) {
                  sm.beamState = obj.beam === 1;
                  sm.onBeamEvent();
              }
      
              if (obj.state != undefined) {
                  if (obj.state === "Idle") {
                      sm.toIdle();
                  } else if (obj.state === "Ready") {
                      sm.toReady();
                  } else if (obj.state === "Set") {
                      sm.toSet();
                  } else if (obj.state === "Go") {
                      sm.toGo();
                  } else if (obj.state === "Finished") {
                      sm.toStop();
                  }
              }
      
              if (obj.mode != undefined) {
                  if (obj.mode === "Stopwatch") {
                      sm.toActive();
                  } else {
                      sm.toInactive();
                  }
              }
      
          },
      
          initButton: function () {
              sm.btn = document.getElementById("btnNext");
              sm.btn.addEventListener('click', sm.onButtonNext);
          },
      
          onLoad: function () {
              sw.init();
              sm.initButton();
              sm.lbl = document.getElementById("lblStatus");
              sm.beam = document.getElementById("beamState");
              sm.initWebSocket();
              sm.toIdle();
              sm.toInactive();
          },
      
          toActive : function() {
              sm.gateActive = true;
              document.getElementById("overlay").style.display = "none";
          },
      
          toInactive : function() {
              sm.gateActive = false;
              document.getElementById("overlay").style.display = "block";
          },
      
          toIdle: function () {
              sw.resetTimer();
              sm.lbl.innerText = sm.statusState = "Idle";
              sm.btn.innerText = "Arm";
              sm.btn.disabled = true;
          },
      
          toReady: function () {
              sm.lbl.innerText = sm.statusState = "Ready";
              sm.btn.disabled = false;
              sm.btn.innerText = "Arm";
          },
      
          toSet: function () {
              sm.lbl.innerText = sm.statusState = "Set";
              sm.btn.disabled = false;
              sm.btn.innerText = "Reset";
          },
      
          toGo: function () {
              sw.startTimer();
              sm.lbl.innerText = sm.statusState = "Go";
              sm.btn.disabled = true;
              sm.btn.innerText = "Reset";
          },
      
          toStop: function () {
              sw.stopTimer();
              sm.lbl.innerText = sm.statusState = "Finished";
              sm.btn.disabled = false;
              sm.btn.innerText = "Reset";
          },
      
          onButtonNext: function () {
              /* // in debug mode, state can come from UI
              
              if (sm.statusState === "Ready") {   // pressed "Arm"
                  sm.toSet();
              } else if (sm.statusState === "Set") {   // pressed "Reset"
                  sm.toReady();
              } else if (sm.statusState === "Finished") {  // pressed "Reset"
                  sw.resetTimer();
                  if (!sm.beamConnected()) {
                      sm.toReady();
                  } else {
                      sm.toIdle();
                  }
              }
              
              */
              //const msg = {event : "apply", state : sm.statusState};
              websocket.send("apply");
          },
      
          onBeamEvent: function () {
              if (sm.beamState) { // connected
                  sm.beam.innerText = "1";
                  sm.beam.style.backgroundColor = "#ff8080";
              } else {
                  sm.beam.innerText = "0";
                  sm.beam.style.backgroundColor = "#808080";
              }
          },
      
          // debug routine to support user beam switching in UI
          onBeamToggle: function() { 
              if (!sm.beamState) {   // re-connect
                  sm.beam.innerText = "1";
                  sm.beam.style.backgroundColor = "#ff8080";
                  if (sm.statusState === "Set") {
                      sm.toGo();
                  } else if (sm.statusState === "Ready") {
                      sm.toIdle();
                  }
              } else {                          // break
                  sm.beam.innerText = "0";
                  sm.beam.style.backgroundColor = "#808080";
                  if (sm.statusState === "Idle") {
                      sm.toReady();
                  } else if (sm.statusState === "Go") {
                      sm.toStop();
                  }
              }
              sm.beamState = !sm.beamState;
          },
      
      };
      window.addEventListener("load", sm.onLoad);
    
    </script>
</head>

<body>
    <div id="overlay"></div>
    <div class="center">
        <div id="sw" class="swtime">
            <div id="MM" class="swcomponent">00</div>:<div id="SS" class="swcomponent">00</div>.<div id="NN" class="swcomponent">00</div>
        </div>
        <div class="swheader">
            <div class="swstatus" id="lblStatus">READY</div>
            <div class="swcontrol">
                <button class="btn nextbtn" id="btnNext">Next</button>
                <div id="beamState">0</div>
            </div>
            
        </div>
    </div>
</body>

</html>
)rawliteral";


// light sensor threshold
float ambient_light_level = 10.0;
float beam_light_level = 50.0;
float light_threshold = 0.0;
int beam_state = 0;

Bounce next_button = Bounce();
Bounce apply_button = Bounce();
bool soft_apply_pressed = false;

enum { MENU_STATUS = 0, MENU_CAL, MENU_WIFI } main_menu_index;
enum { STATUS_IDLE = 0, STATUS_READY, STATUS_SET, STATUS_STOPWATCH, STATUS_LAP_TIME } status_display_mode;
enum { CAL_TOP = 0, CAL_AMBIENT, CAL_BEAM, CAL_BACK } cal_menu_index;
enum { WIFI_TOP = 0, WIFI_EN, WIFI_INFO, WIFI_BACK } wifi_menu_index;

hw_timer_t* timer = NULL;
volatile unsigned long lap_time_ms = 0;
void IRAM_ATTR on_timer() {
    lap_time_ms += 10;
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
    display.setCursor(8,48);
    display.println(F("..Go!"));
    display.display();
}

// hand roll some Json
String buildMessage(bool const syncTime) {
  String msg = "{";
  if (syncTime) {
    msg += "\"sync_elapsed_ms\":";
    msg += lap_time_ms;
    msg += ",";
  }
  msg += "\"beam\":";
  msg += beam_state;
  if (main_menu_index == MENU_STATUS) {
    msg += ",\"state\":";
    //enum { STATUS_IDLE = 0, STATUS_READY, STATUS_SET, STATUS_STOPWATCH, STATUS_LAP_TIME } status_display_mode;
    if (status_display_mode == STATUS_IDLE) {
      msg += "\"Idle\"";
    } else if (status_display_mode == STATUS_READY) {
      msg += "\"Ready\"";
    } else if (status_display_mode == STATUS_SET) {
      msg += "\"Set\"";
    } else if (status_display_mode == STATUS_STOPWATCH) {
      msg += "\"Go\"";
    } else if (status_display_mode == STATUS_LAP_TIME) {
      msg += "\"Finished\"";
    } else {
      msg += "\"Undefined\""; // error
    }
  }
  msg += ",\"mode\":";
  if (main_menu_index == MENU_STATUS) {
    msg += "\"Stopwatch\"";
  } else if (main_menu_index == MENU_CAL) {
    msg += "\"Calibration\"";
  } else if (main_menu_index == MENU_WIFI) {
    msg += "\"Wifi\"";
  } else {
    msg += "\"Undefined\""; // error
  }
  msg += "}";
  Serial.println(msg);
  return msg;
  
}

// broadcast to all connected clients
void notifyClients(bool const syncTime) {
  ws.textAll(buildMessage(syncTime));
}

// client sent a message: apply button pressed
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "apply") == 0) {
      soft_apply_pressed = true;
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) 
{
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      ws.text(client->id(),buildMessage(true)); 
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

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
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


    // Connect to Wi-Fi (TODO: replace as AP)
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi..");
    }

    // Print ESP Local IP Address
    Serial.println(WiFi.localIP());

    initWebSocket();

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", index_html);
    });

    // Start server
    server.begin();
    Serial.println("Started server");
    
}


char time_string_buf[16];
char sensor_string_buf[16];
char wifi_string_buf[16];

void loop() 
{
    ws.cleanupClients();
    
    float const raw_val = (4095 - analogRead(OPTICAL_GATE_SENSE_PIN)) / 40.95;  // 0 - 100
    if (raw_val > light_threshold) {  // beam on
        bool do_notify = beam_state != 1;       
        beam_state = 1;
        digitalWrite(OPTICAL_GATE_STATUS_PIN, HIGH);
        if (main_menu_index == MENU_STATUS) {
            if (status_display_mode == STATUS_READY) {  // was not armed
                status_display_mode = STATUS_IDLE;
                do_notify = true;
            } else if (status_display_mode == STATUS_SET) { // athlete started
                status_display_mode = STATUS_STOPWATCH;
                digitalWrite(OPTICAL_GATE_ARMED_PIN, LOW);
                timerAlarmEnable(timer);
                do_notify = true;
            }
            if (do_notify) {
              String const msg = buildMessage(false);
              notifyClients(msg);
            }
        }
    } else {  // beam off
        bool do_notify = beam_state != 0;
        beam_state = 0;
        digitalWrite(OPTICAL_GATE_STATUS_PIN, LOW);
        if (main_menu_index == MENU_STATUS) {
            bool sync_clock = false;
            if ((status_display_mode == STATUS_STOPWATCH) && (lap_time_ms > 500)) {  // athlete finishing, debounced
                status_display_mode = STATUS_LAP_TIME;
                timerEnd(timer);
                timer = NULL;
                do_notify = true;
                sync_clock = true;
            } else if (status_display_mode == STATUS_IDLE) {  // athlete ready, not armed
                status_display_mode = STATUS_READY;    
                do_notify = true;
            }
            if (do_notify) {
              String const msg = buildMessage(sync_clock);
              notifyClients(msg);
            }
        }
    }

    next_button.update();
    if (next_button.fell()) {
        bool sync_mode = false;  
        if (main_menu_index == MENU_STATUS) {
            if (status_display_mode != STATUS_STOPWATCH) {
                if ((status_display_mode == STATUS_SET) || (status_display_mode == STATUS_LAP_TIME)) {
                    status_display_mode = STATUS_IDLE;
                    digitalWrite(OPTICAL_GATE_ARMED_PIN, LOW);
                }
                main_menu_index = MENU_CAL;
                sync_mode = true;
            }
        } else if (main_menu_index == MENU_CAL) {
            if (cal_menu_index == CAL_TOP) {
                main_menu_index = MENU_WIFI;
                sync_mode = true;
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
                sync_mode = true;
            } else if (wifi_menu_index == WIFI_BACK) {
                wifi_menu_index = WIFI_EN;
            }  else if (wifi_menu_index == WIFI_EN) {
                wifi_menu_index = WIFI_INFO;
            }  else if (wifi_menu_index == WIFI_INFO) {
                wifi_menu_index = WIFI_BACK;
            }
        }
        if (sync_mode) {
          String const msg = buildMessage(false);
          notifyClients(msg);
        }
    }

    apply_button.update();
    if (apply_button.fell() || soft_apply_pressed) {
        soft_apply_pressed = false;
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
                lap_time_ms = 0;
            }
            String const msg = buildMessage(false);
            notifyClients(msg);
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
            unsigned long current_lap_time = lap_time_ms;
            unsigned long const mins = lap_time_ms / 60000;
            current_lap_time %= 60000;
            unsigned long const secs = current_lap_time / 1000;
            current_lap_time %= 1000;
            current_lap_time /= 10; // to hundredths
            sprintf(time_string_buf, "%02d:%02d.%02d",mins, secs, current_lap_time);
            display.println(time_string_buf);
        } else if (status_display_mode == STATUS_LAP_TIME) {
            unsigned long current_lap_time = lap_time_ms;
            unsigned long const mins = lap_time_ms / 60000;
            current_lap_time %= 60000;
            unsigned long const secs = current_lap_time / 1000;
            current_lap_time %= 1000;
            current_lap_time /= 10; // to hundredths
            sprintf(time_string_buf, "%02d:%02d.%02d<",mins, secs, current_lap_time);
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
