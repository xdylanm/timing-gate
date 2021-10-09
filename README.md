# timing-gate
An optical timing gate for sports

## Parts List

* ESP32 development board ([link](https://www.amazon.ca/gp/product/B07QCP2451))
* SSD1306 128x64 pixel I2C OLED display ([link](https://www.amazon.ca/CANADUINO-OLED-Display-128x64-Pixel/dp/B0751LFCZT))
* 5V 6mm 650nm (red) diode laser ([link](https://www.amazon.ca/gp/product/B0833Z1WG4))
* Photoresistor ([GL5539](https://www.amazon.ca/gp/product/B0833Z1WG4))
* 10k and 220-330 ohm resistors
* 2 momentary-on SPDT buttons

## References

1. https://randomnerdtutorials.com/projects-esp32/
2. https://randomnerdtutorials.com/esp32-websocket-server-arduino/

## User Interface
The user interface is provided through an onboard OLED display (128x64) with two buttons, and three status LEDs. The interface is mirrored in an HTML page that is accessible through a browser on a device connected to the AP. The two buttons function as 

* Mode (Next): advance to the next action
* Select (Apply): do the selected action

### Menu Flow

1. Status: displays "Idle", "Ready", "Set", stopwatch time, lap time
   * Idle: beam is on sensor, waiting
     * [apply] does nothing
     * [next] to Calibration
   * Ready: beam is blocked (athlete is in starting position), not armed
     * [apply] arms the gate, moves status to "Set" 
     * [next] to Calibration
   * Set: beam is blocked (athlete is in starting position), armed
     * [apply] disarms the gate, back to "Ready"
     * [next] disarms the gate, back to "Ready", to Calibration
   * stopwatch time: displays running stopwatch starting when the beam is restored (athlete leaves starting position) 
     * [apply] does nothing
     * [next] does nothing
   * lap time: when the beam is broken again (athlete completes lap), stopwatch stops, and the display shows the elapsed time
     * [apply] clears time, returns to Idle or Ready (beam is on/off the sensor)
     * [next] clears time, to Calibration
2. Calibration
   * [apply] enters menu
     * Ambient: displays raw sensor level (0-100)
       * [apply] uses current value as ambient level (beam not on sensor, but sensor exposed)
       * [next] to Beam
     * Beam: displays raw sensor level (0-100)
       * [apply] uses current value as beam-on-sensor level
       * [next] to Back
     * Back:
       * [apply] to Calibration
       * [next] to Ambient
   * [next] to WiFi
3. WiFi
   * [apply] enters menu
     * Enable: displays Wifi status (en/dis)
       * [apply] switches on/off
       * [next] to Info
     * Info: displays Wifi SSID & Password
       * [apply] locks or unlocks ability to remote change password (through web interface) 
       * [next] to Back
     * Back:
       * [apply] to Wifi
       * [next] to Enable
   * [next] to Status

### Status LEDs

#### Beam State

The first status LED tracks the state of the beam: ON indicates that the beam is illuminating the sensor, OFF indicates that the beam is disrupted.  

#### Armed State

The second status LED indicates when the gate is armed (athlete is in position to start, beam is disrupted). When the athlete starts and the beam re-connects with the sensor, the stopwatch starts and the armed state is reset.

## Web Interface

The web interface is provided via WebSockets, which enables asynchronous and near-real-time updates between the ESP and multiple connected clients. Client side behaviour is handled locally through JS, and state is held on the server. 

* The clients will use their own timers for the display, relying on the server to provide time synchronization: the time displayed on the client's browser may not match the stopwatch time on the timing gate, but will be updated at the end of a lap to reflect the correct time. 

* Clients can arm and reset the stopwatch ("apply" action) remotely (TODO - only give selected clients "admin" control). 

* The server communicates state via JSON encoded messages. The key parts to the message are

  * sync_elapsed_ms: (optional) the current elapsed stopwatch time on the gate in milliseconds
  * beam: the status of the sensor (1=beam on sensor, 0=beam disconnected)
  * state: the state in the stopwatch flow (Idle, Ready, Set, Go, Finished)
  * mode: the menu mode on the gate (Stopwatch, Calibration, Wifi). When not in Stopwatch mode, the web interface is disabled.

  An example message

  ```json
  {"sync_elapsed_ms":1560,"beam":1,"state":"Finished","mode":"Stopwatch"}
  ```

#### Useful Javascript Stopwatch Examples

* https://code-boxx.com/simple-javascript-stopwatch/
* https://dev.to/gspteck/create-a-stopwatch-in-javascript-2mak
* https://jsfiddle.net/Larph/he10jyu9/

#### WebSocket References

* https://randomnerdtutorials.com/esp32-websocket-server-arduino/
* https://github.com/me-no-dev/ESPAsyncWebServer

## WiFi AP

The timing gate can act as a WiFi access point (AP) with a SSID. The timing gate will host a web page that can display results and update settings.





