/** TODO
 *      - Create Reactive table
 *      - Test by hardcoding additional values
 *      - Look at integrating Live View as well 
 */ 
api_version = '0.1'

$( document ).ready(function() 
{
});


function round3dp(flt)
{
    return flt.toFixed(3);
}

function update_live_view_frame(frame_data)
{
    return `
        <div>  
            <table>
                <colgroup>
                    <col>
                    <col>
                </colgroup>
                <tbody>
                    <tr>
                        <th> Frame </th>
                        <td> ${frame_data['frame_number']} </td>
                    </tr>
                    <tr>
                        <th> Processing Time </th>
                        <td> ${frame_data['process_time']} ms</td>
                    </tr>
                    <tr>
                        <th> Result Weight 1: </th>
                        <td> ${frame_data['result'][0].toFixed(4)}</td>
                    </tr>
                    <tr>
                        <th> Result Weight 2: </th>
                        <td> ${frame_data['result'][1].toFixed(4)}</td>
                    </tr>
                </tbody>
            </table>
        </div>
        `;
}

function update_past_frames(frame_past_data)
{


    return `
        <div>
            <table>
                <tr>
                    <th>Frame</th>
                    <td>${frame_numbers_header_html}</td>
                </tr>
                <tr>
                    <th>Result Weight 1:</th>
                    <td>${frame_classification_html}</td>
                </tr>
                <tr>
                    <th>Result Weight 2:</th>
                    <td>${frame_certainty_html}</td>
                </tr>
            </table>
        </div>
        `;
}

function update_frame_data() {

    $.getJSON('/api/' + api_version + '/inaira/', function(response) {
        frame_data = response.frame_data;
        console.log(response.frame_data)
        $("#current_frame_data").html(update_live_view_frame(frame_data));
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
 