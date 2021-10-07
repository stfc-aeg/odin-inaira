api_version = '0.1'

function update_frame_data() {

    $.getJSON('/api/' + api_version + '/inaira/', function(response) {
        if (response.frame.frame_number != null){
            $('#frame_number').html(response.frame.frame_number);
            $('#process_time').html(response.frame.process_time + "ms");
            $('#result').html(response.frame.certainty.toFixed(4));
            $('#classification').html(response.frame.classification);
            $('#pass_ratio').html(response.experiment.pass_ratio.toFixed(8));
            $('#avg_process_time').html(response.experiment.avg_processing_time.toFixed(2) + "ms");
            $('#total_frames').html(response.experiment.total_frames);
        }
        else {
            console.log("Frame data is null")
        }
        
    });
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
 