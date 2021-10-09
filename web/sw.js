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

    timerCycle: function () {
        if (sw.running) {
            const t = performance.now() * 0.001;  // millis
            const dt = t - sw.tStart;
            //pt = t;
            const tt = Math.floor(dt);
            const mil = dt - tt;
            sw.lblHun.textContent = (mil).toFixed(2).slice(-2);
            const sec = tt % 60;
            //if (sec == ps) return;
            //ps = sec;
            const min = Math.floor(tt / 60) % 60;
            sw.lblMin.textContent = ('0' + min).slice(-2);
            sw.lblSec.textContent = ('0' + sec).slice(-2);
        }
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
            sw.tStart = performance.now() * 0.001;
            sw.intervalId = setInterval(sw.timerCycle, 31);
        }
    }
};



// state machine
const sm = {
    btn: null,
    lbl: null,
    beam :null,
    statusState: "",
    
    init: function () {
        sw.init();
        sm.btn = document.getElementById("btnNext");
        sm.lbl = document.getElementById("lblStatus");
        sm.beam = document.getElementById("beamState");
        sm.toIdle();
    },

    toIdle: function () {
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
    },

    onBeamToggle: function () {
        //console.log(sm.beam.innerText);
        if (!sm.beamConnected()) {   // re-connect
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
    },

    beamConnected : function() {
        return sm.beam.innerText === "1";
    }

};



window.addEventListener("load", sm.init);
