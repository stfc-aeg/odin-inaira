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

function state_up(){

}

function state_down(){

}

function get_config(){
    $.getJSON('/api/' + api_version + '/inaira/', function(response){
        camera_config = response.camera_control.config.camera;
        
        get_element_by_id("frame_rate").value = camera_config.frame_rate;
        get_element_by_id("exposure_time").value = camera_config.exposure_time;
        get_element_by_id("num_frames").value = camera_config.num_frames;
        get_element_by_id("image_timeout").value = camera_config.image_timeout;
        get_element_by_id("camera_num").value = camera_config.camera_num;
    });
}

function send_config(){
    // Put config updates
    $.ajax({
        type: "PUT",
        url: '/api/' + api_version + '/inaira/camera_control/config/camera/',
        contentType: "application/json",
        data: '{"frame_rate" : ' + parseInt(get_element_by_id("frame_rate").value) + ','
                + '"exposure_time" : ' + get_element_by_id("exposure_time").value + ','
                + '"num_frames" : ' + get_element_by_id("num_frames").value + ','
                + '"image_timeout" : ' + get_element_by_id("image_timeout").value + ','
                + '"camera_num" : ' + get_element_by_id("camera_num").value + '}'
    });
    
}

function get_element_by_id(element_id){
    return document.getElementById(element_id);
}

function get_status(){
    
}

function change_enable() {
    var enabled = $('#task-enable').prop('checked');
    console.log("Enabled changed to " + (enabled ? "true" : "false"));
    $.ajax({
        type: "PUT",
        url: '/api/' + api_version + '/workshop/background_task',
        contentType: "application/json",
        data: JSON.stringify({'enable': enabled})
    });
}
