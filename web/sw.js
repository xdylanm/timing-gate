const sw = {
    btn : null,
    lbl : null,

    init : function() {
        console.log('sw.init()');
        sw.btn = document.getElementById("nextButton");
        sw.lbl = document.getElementById("statuslbl");
    },

    onButtonNext : function () {
        if (sw.btn.innerText === "Next") {
            sw.btn.innerText = "Reset";
            sw.lbl.innerText = "Rocket";
        } else {
            sw.btn.innerText = "Next";
            sw.lbl.innerText = "Ready";
        }
    }

};

window.addEventListener("load",sw.init);
