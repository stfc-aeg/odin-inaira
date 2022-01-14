api_version = '0.1';
page_load = true;
camera_config = {};
camera_status = {};
local_config = {};

/**
 *  TODO:
 *      - Disable/Grey out/Remove state change buttons that will do nothing.
 *      
 * 
 */


//#region On Page Ready

/**
 * Run this when the document is ready.
 */

$( document ).ready(function() {

    // First get the values from the adapter
    get_config();
    get_status();
    change_state("_innit_");
    
    get_timer();
});

/**
 * Continuous poll timer
 */

 function get_timer() {
    setTimeout(get_these, 1000);
}

function get_these() {
    get_config();
    get_status();
    get_timer();
}

//#endregion

//#region Get Functions

/**
 * Get the camera config and set locally.
 */
function get_config(){
    $.getJSON('/api/' + api_version + '/inaira/', function(response) {
        adapter_camera_config = response.camera_control.config.camera;

        camera_config = JSON.parse(JSON.stringify(return_evaluated_object(adapter_camera_config)));
        
        // This has to be done AFTER the camera_config has been updated.
        // If this is the first time the page is loading. Create the change index for the config.
        if(page_load){
            // Create marker for changed inputs, with False to indicate no change.
            Object.entries(camera_config).forEach(entry => {
                const [key, value] = entry;
                local_config[key] = JSON.parse('{"value":' + value + ', "changed": false}');
            });
            update_config();
            page_load = false;
        }
    });
    console.log("Got and updated config");
}

/**
 * Get the camera status and set locally.
 */

function get_status(){
    $.getJSON('/api/' + api_version + '/inaira/', function(response) {
        adapter_camera_status = response.camera_control.status;

        camera_status = JSON.parse(JSON.stringify(return_evaluated_object(adapter_camera_status)));
        update_status();
        update_state_buttons();
    }
    );
    console.log("Got and updated status");
}

/**
 * Get the config values shown in the UI and set locally.
 */
function get_local_config_values(){
    local_config.frame_rate.value = get_element_by_id("frame_rate").value;
    local_config.exposure_time.value = get_element_by_id("exposure_time").value;
    local_config.num_frames.value = get_element_by_id("num_frames").value;
    local_config.image_timeout.value = get_element_by_id("image_timeout").value;
    local_config.camera_num.value = get_element_by_id("camera_num").value;
}

/**
 * Updates the frame data UI display for the current frame.
 */
function update_frame_data() {

    $.getJSON('/api/' + api_version + '/inaira/', function(response) {
        if (response.odin_inaira.frame.frame_number != null){
            $('#frame_number').html(response.odin_inaira.frame.frame_number);
            $('#process_time').html(response.odin_inaira.frame.process_time + "ms");
            $('#result').html(response.odin_inaira.frame.certainty.toFixed(4));
            $('#classification').html(response.odin_inaira.frame.classification);
            $('#pass_ratio').html(response.odin_inaira.experiment.pass_ratio.toFixed(8));
            $('#avg_process_time').html(response.odin_inaira.experiment.avg_processing_time.toFixed(2) + "ms");
            $('#total_frames').html(response.odin_inaira.experiment.total_frames);
        }
        else {
            console.log("Frame data is null")
        }
    });
}

//#endregion

//#region Display Update Functions

/**
 * 
 * `update_config()`:
 *      Updates the config UI display, checks if changes have been made or are being made and will not overwrite these.
 * 
 * `update_status():
 *      Updates the status UI display whenever called.
 * 
 * `refresh_config():
 *      Refreshes the config UI display to the camera_config values. In line with the adapter and camera.
 */

/**
 * Update the camera Config display.
 */
 function update_config(){
    Object.entries(local_config).forEach(entry => {
        const [key, entry_value] = entry;
        // Timestamp mode is node exposed to the user.. should probably filter this out during the creation of local copy... meta data can do this
        if (key != 'timestamp_mode'){      
            // Is it currently being edited
            if (!(get_focused_element() == key)){
                // Is it changed?
                if (!(entry_value.changed) && entry_value != null) {
                    get_element_by_id(key).value = entry_value.value;
                }
            }
        }
    });
}

/** 
 * Update the camera Status display.
 */

function update_status(){
    Object.entries(camera_status).forEach(entry => {
        const [key, entry_value] = entry;
        get_element_by_id(key).innerText = entry_value;
    });
}

/**
 * Refresh the Config display to camera settings.
 */
function refresh_config(){
    Object.entries(camera_config).forEach(entry => {
        const [key, value] = entry;
        get_element_by_id(key).value = value;       
    });

    // Reset the local copy of config to default.
    Object.entries(local_config).forEach(entry => {
        const [key, value] = entry;
        if (value.changed){
            local_config[key]["changed"] = false;
            local_config[key]["value"] = camera_config[key];
        }
    });
    console.log("Refreshed config to camera level");
}

/**
 * Will update the state display buttons.
 */
function update_state_buttons(){
    $.getJSON('/api/' + api_version + '/inaira/camera_control/status_change/', function(response){
        up_button = get_element_by_id("stateUp");
        down_button = get_element_by_id("stateDown");
        up_button.innerText = response.status_change.up_button_text;
        up_button.enabled = response.status_change.up_button_enabled;
        down_button.innerText = response.status_change.down_button_text;
        down_button.enabled = response.status_change.down_button_enabled;
    });
}

//#endregion

//#region Button functions that do not update the UI.

/**
 * Notify that a input has been changed.
 */
function config_input_changed(element_id){
    // Change assosicate input change to true.
    local_config[element_id]["changed"] = true;
    local_config[element_id]["value"] = parseInt(get_element_by_id(element_id).value);
}

/**
 * Sends all changed config to the adapter
 */
function send_config(){
    // Put config updates
    config_data = {};

    // Checks if the "changed" value is true on the local copy.
    Object.entries(local_config).forEach(entry => {
        const [key, value] = entry;
        if (value.changed){
            // Now compare with the original
            if (!(value.value == camera_config[key])){
                // Add it to the list of values to send.
                config_data[key] = value.value;

                // Reset local config to not changed
                local_config[key]["changed"] = false;
            }
        }
    });

    if (Object.keys(config_data).length > 0){
        $.ajax({
            type: "PUT",
            url: '/api/' + api_version + '/inaira/camera_control/config/camera/',
            contentType: "application/json",
            data: JSON.stringify(config_data)
        });
    } else {
        console.log("No changes were made. Not sending PUT request");
    }

}

/**
 * Sends the change of state.
 */
function change_state(change){
    $.ajax({
        type: "PUT",
        url: '/api/' + api_version + '/inaira/camera_control/status_change',
        contentType: "application/json",
        data: '{"change": "' + change + '"}'
    });
}

//#endregion

//#region Functions called from within odin_server.js

/**
 * Get values from the document.
 */
function get_element_by_id(element_id){
    return document.getElementById(element_id);
}

/**
 * Return the ID of the focused element.
 */
function get_focused_element(){
    return document.activeElement;
}
/**
 * Evaluate object for other objects. 
 */
function return_evaluated_object(object_to_evaluate){
    return_value = {};
    loop = true;
    evaluate = object_to_evaluate;
    while (loop) {
        new_evaluation = {};
        Object.entries(evaluate).forEach(entry => {
            const [key, entry_value] = entry;
            // Not an Object
            if (typeof entry_value !== 'object'){
                return_value[key] = entry_value;
                delete evaluate[key];
            } 

            // An Object
            if (typeof entry_value === 'object') {
                new_value = entry_value;
                Object.entries(new_value).forEach(new_entry => {
                    const [new_key, new_value] = new_entry;
                    new_evaluation[new_key] = new_value;
                });
            }
        });

        if (Object.keys(evaluate).length == 0){
            loop = false;
        }

        evaluate = JSON.parse(JSON.stringify(new_evaluation));
    }

    return return_value;
}



//#endregion
