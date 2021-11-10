api_version = '0.1';

$( document ).ready(function() {

    update_api_version();
    update_api_adapters();
});

function update_api_version() {

    $.getJSON('/api', function(response) {
        $('#api-version').html(response.api);
        api_version = response.api;
    });
}

function update_api_adapters() {

    $.getJSON('/api/' + api_version + '/adapters/', function(response) {
        adapter_list = response.adapters.join(", ");
        $('#api-adapters').html(adapter_list);
    });
}

function start_stop() {
    if (document.getElementById("startStop").className == "button_green") {
        document.getElementById("startStop").className = "button_red";
        console.log("Change Start / Stop to Red");
    } 
    else {
        document.getElementById("startStop").className = "button_green";
        console.log("Change Start / Stop to Green");
    }
    
}

function arm_disarm(){
    if (document.getElementById("armDisarm").className == "button_green") {
        document.getElementById("armDisarm").className = "button_red";
        console.log("Change Arm / Disarm to Red");
    } 
    else {
        document.getElementById("armDisarm").className = "button_green";
        console.log("Change Arm / Disarm to Green");
    }

    /**
     * Need to edit a json configuration that can be sent to the adapter then to the camera..
     * 
     * using a putJSON ? 
     * Set up parameter tree in python
     * 
     */
}