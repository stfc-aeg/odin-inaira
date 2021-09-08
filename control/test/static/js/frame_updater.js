/** TODO
 *      - Create Reactive table
 *      - Test by hardcoding additional values
 *      - Look at integrating Live View as well 
 */ 
api_version = '0.1'


self.good_frames;

$( document ).ready(function() 
{
    self.good_frames = 0;
});


function round3dp(flt)
{
    return flt.toFixed(3);
}

function update_live_view_frame(frame_data)
{
    
}

function update_frame_data() {

    $.getJSON('/api/' + api_version + '/inaira/', function(response) {
        $('#frame_number').html(response.frame.frame_number);
        $('#process_time').html(response.frame.process_time + "ms");
        $('#result').html(response.frame.certainty.toFixed(4))
        $('#classification').html(response.frame.classification);
        $('#pass_ratio').html(response.pass_ratio.toFixed(4));
        $('#avg_process_time').html(response.avg_processing_time.toFixed(2) + "ms");
        $('#total_frames').html(response.total_frames);
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
 