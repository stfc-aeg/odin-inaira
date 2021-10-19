import logging
import sys
import os
import json

from json.decoder import JSONDecodeError

from odin_data.ipc_channel import IpcChannel, IpcChannelException
from odin_data.ipc_message import IpcMessage, IpcMessageException

from odin.adapters.adapter import (ApiAdapter, ApiAdapterRequest,
                                   ApiAdapterResponse, request_types, response_types)
from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError
from odin.util import decode_request_body


class CameraControlAdapter(ApiAdapter):

    def __init__(self, **kwargs):
        super(CameraControlAdapter, self).__init__(**kwargs)

        ctrl_endpoint = self.options.get("ctrl_endpoint", "")
        self.controller = CameraController(ctrl_endpoint)

    @response_types('application/json', default='application/json')
    def get(self, path, request):
        try:
            response, content_type, status = self.controller.get(path, request)
        except ParameterTreeError as param_error:
            response = {'response': 'CameraControl GET error: {}'.format(param_error)}
            content_type = 'application/json'
            status = 400
    
        return ApiAdapterResponse(response, content_type=content_type, status_code=status)

    @response_types('application/json', default='application/json')
    def put(self, path, request):
        try:
            data = decode_request_body(request)
            self.controller.put(path, data)
            response, content_type, status = self.controller.get(path, request)
        except ParameterTreeError as param_error:
            response = {'response': 'CameraControl PUT error: {}'.format(param_error)}
            content_type = 'application/json'
            status = 400
        
        return ApiAdapterResponse(response, content_type=content_type, status_code=status)

class CameraController():

    # Define the maximum message ID for the IPC channel
    MESSAGE_ID_MAX = 2**32

    def __init__(self, ctrl_endpoint):

        # init control IPC channel and connect to endpoint
        self.ctrl_endpoint = ctrl_endpoint
        self.ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_DEALER)
        self.ctrl_channel.connect(self.ctrl_endpoint)
        logging.debug("Connected to camera at endpoint: %s", self.ctrl_endpoint)

        # Init the message id counter and timeout
        self._msg_id = 0
        self._timeout_ms = 1000
        self.command_response = ""
        self.config_file = ""

        self.param_tree = ParameterTree({
            "name": "Camera Control Adapter",
            "ctrl_endpoint": (self.ctrl_endpoint, None),
            "status": (self.get_status, None),
            "config": (self._request_config, self._send_config),
            "config_file": (lambda: self.config_file, self.send_config_file),
            "command": (lambda: self.command_response, self.do_command)
        })

    def get(self, path, request):

        response = self.param_tree.get(path)
        content_type = "application/json"
        status = 200
        return response, content_type, status

    def put(self, path, data):

        self.param_tree.set(path, data)

    def _next_msg_id(self):
        """Return the next message ID for the control channel
        
        This internal method increments and returns the next value of the message
        control IPC channel.
        """

        self._msg_id = (self._msg_id + 1) % self.MESSAGE_ID_MAX
        return self._msg_id

    def do_command(self, command):
        """Send a control command to the PCO camera controller.

        This method sends a control command to the PCO camera. This is implemented as an IPC
        configuration message with the appropriate camera command in the payload.

        :param command: string camera control command
        """
        params = {"command": command}
        logging.debug("Doing Command: {}".format(params))
        reply = self._send_cmd("configure", params)
        self.command_response = reply.attrs
        # logging.info(f"Command response: \n{self.format_json(reply)}")

    def send_config_file(self, json_path=None):
        """Send or request configuration parameters to/from the PCO camera controller.

        This method sends or requests configuration parameters to/from the PCO camera controller.
        These can either by provided in a JSON file at the specified path, which will be loaded and
        injected into the configuration message, or as individual key-value pairs. If neither a file
        nor any key-value pairs are specified, request the current configuration instead

        :param json_path: optional JSON config file path to load
        :param json_vals: tuple of key-value pairs to parse into JSON fields
        """

        # Create the appropriate parameter payload to send to the controller
        params = {"camera": {}}

        # If a json file path has been specified, attempt to load that file
        try:
            with open(json_path) as json_file:
                params["camera"] = json.load(json_file)
            self.config_file = json_path
        except JSONDecodeError as json_error:
            logging.error(f"Failed to decode JSON config file: {json_error}")
            return
        
        self._send_config(params)

    def _send_config(self, params=None):
        """Set PCO camera controller confiugration parameters.

        This internal method sends a configuration command to the PCO camera controller, with
        the specified parameter dict as a payload.

        :param params: dictionary of parameters to add to the IPC channel message.
        """
        reply = self._send_cmd("configure", params)
        # logging.info(f"Configuration response: \n{self.format_json(reply)}")

    def _request_config(self):
        """Get the configuration of the PCO camera controller.

        This interal method sends a configuration request command to the PCO camera controller and
        displays the response as formatted JSON.
        """
        reply = self._send_cmd("request_configuration")
        # logging.info(f"Config request response: \n{self.format_json(reply)}")

    def get_status(self):
        """Get the status of the PCO camera controller.

        This method send a status request command to the PCO camera controller and displays
        the response as formatted JSON.
        """
        reply = self._send_cmd("status")
        # logging.info(f"Status response: \n{self.format_json(reply)}")
        return reply.attrs

    def _send_cmd(self, cmd, params=None):
        """Send an IPC command message to the PCO camera controller.

        This internal method sends an IPC command message to the PCO camera controller, with the
        specified parameter dict as the payload.

        :param cmd: string command
        """
        cmd_msg = IpcMessage("cmd", cmd, id=self._next_msg_id())
        if params:
            cmd_msg.attrs["params"] = params

        self.ctrl_channel.send(cmd_msg.encode())

        reply = None
        pollevts = self.ctrl_channel.poll(self._timeout_ms)
        if pollevts == IpcChannel.POLLIN:
            reply = self.ctrl_channel.recv()

        return IpcMessage(from_str=reply)