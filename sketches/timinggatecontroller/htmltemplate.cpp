#include "htmltemplate.h"

namespace HTML
{

const char PROGMEM index[] = R"rawliteral(
<!DOCTYPE html>
<html>

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
              // TODO - when the stopwatch is not running, this has no effect
              //        need to handle the case where we haven't started yet
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

} // namespace HTML
