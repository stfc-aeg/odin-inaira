api_version = '0.1';

$( document ).ready(function() {

    update_api_version();
    update_api_adapters();
    poll_update()
});

function poll_update() {
    update_test_counter();
    setTimeout(poll_update, 500);   
}

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

function update_test_counter() {

    $.getJSON('/api/' + api_version + '/inaira/', function(response) {
        var test_counter = response.frame.test_counter;
        $('#test_counter').html(test_counter);
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
