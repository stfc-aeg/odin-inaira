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

from tornado.ioloop import PeriodicCallback

class CameraController():

    # Define the maximum message ID for the IPC channel
    MESSAGE_ID_MAX = 2**32

    def __init__(self, ctrl_endpoint, status_refresh):

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

        self.connected = False

        self.frame_timeout_ms = 1000
        self.camera_num = 0
        self.exposure_time = 0.01
        self.frame_rate = 100
        self.image_timeout = 10.0
        self.num_frames = 10

        camera_config = self._request_config()
        if camera_config['msg_type'] == "ack":
            camera_config = camera_config['params']
            
            self.frame_timeout_ms = camera_config['frame_timeout_ms']
            camera_config = camera_config['camera']
            self.camera_num = camera_config['camera_num']
            self.exposure_time = camera_config['exposure_time']
            self.frame_rate = camera_config['frame_rate']
            self.image_timeout = camera_config['image_timeout']
            self.num_frames = camera_config['num_frames']

        self.status_tree = ParameterTree({
            "connected": (lambda: self.connected, None)
        }, mutable=True)

        self.config_tree = ParameterTree({
            "enable_packet_logging": (False, None),
            "frame_timeout_ms": (lambda: self.frame_timeout_ms, None),
            "camera": {
                "camera_num": (lambda: self.camera_num,       lambda a: self._check_config(self.camera_num, {'camera_num': a}, a)),
                "exposure_time": (lambda: self.exposure_time, lambda a: self._check_config(self.exposure_time, {'exposure_time': a}, a)),
                "frame_rate": (lambda: self.frame_rate,       lambda a: self._check_config(self.frame_rate, {'frame_rate': a}, a)),
                "image_timeout": (lambda: self.image_timeout, lambda a: self._check_config(self.image_timeout, {"image_timeout": a}, a)),
                "num_frames": (lambda: self.num_frames,       lambda a: self._check_config(self.num_frames, {"num_frames": a}, a))
            }
        })

        self.param_tree = ParameterTree({
            "name": "Camera Control Adapter",
            "ctrl_endpoint": (self.ctrl_endpoint, None),
            "status": self.status_tree,
            "config": self.config_tree,
            "config_file": (lambda: self.config_file, self.send_config_file),
            "command": (lambda: self.command_response, self.do_command)
            # "request_config": (self._request_config, None)
        })

        self.background_task = PeriodicCallback(
            self.get_camera_state, status_refresh  # Get Camera State once every second
        )
        self.background_task.start()

    def get(self, path, request):

        response = self.param_tree.get(path)
        content_type = "application/json"
        status = 200
        return response, content_type, status

    def put(self, path, data):

        self.param_tree.set(path, data)

    def get_camera_state(self):
        # logging.debug("GETTING CAMERA STATE")
        reply = self._send_cmd("status")
        # logging.debug("MESSAGE TYPE: {}".format(reply.get_msg_type()))
        # logging.debug(reply.ACK)
        # logging.debug(reply)
        try:
            self.connected = reply.get_msg_type() and reply.get_msg_type() in reply.ACK
            # logging.debug("CONNECTED: {}".format(self.connected))
            self.status_tree.set('', {"acquisition": reply.get_params().get("acquisition", {})})
            self.status_tree.set('', {"camera": reply.get_params().get('camera', {})})
        except KeyError as err:
            logging.error("Key Not Found: {}".format(err))

        reply = self._send_cmd("request_configuration")

        try:
            self.frame_timeout_ms = reply.get_params().get("frame_timeout_ms", self.frame_timeout_ms)
            camera_config = reply.get_params().get("camera", {})
            self.camera_num = camera_config.get("camera_num", self.camera_num)
            self.exposure_time = camera_config.get("exposure_time", self.exposure_time)
            self.frame_rate = camera_config.get("frame_rate", self.frame_rate)
            self.image_timeout = camera_config.get("image_timeout", self.image_timeout)
            self.num_frames = camera_config.get("num_frames", self.num_frames)

        except KeyError as err:
            logging.error("Key Not Found: {}".format(err))

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
        """Send configuration parameters to the PCO camera controller.

        This method sends or requests configuration parameters to/from the PCO camera controller.
        This method does this using configuration provided in a JSON file at the specified path,
        which will be loaded and injected into the configuration message.

        :param json_path: optional JSON config file path to load
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

    def _check_config(self, config_param, params, a):
        if (config_param == a):
            logging.debug("CONFIG PARAMETER NOT CHANGED")
        else:
            self._send_config(params)

    def _send_config(self, params=None):
        """Set PCO camera controller confiugration parameters.

        This internal method sends a configuration command to the PCO camera controller, with
        the specified parameter dict as a payload.

        :param params: dictionary of parameters to add to the IPC channel message.
        """
        logging.debug("SEND CONFIG PARAMS:\n{}".format(params))
        if type(params) is dict:
            if "camera" not in params:
                params = {'camera': params}
            logging.debug("SENDING PARAMS FROM PUT REQUEST")
            reply = self._send_cmd("configure", params)
        else:
            logging.debug("SENDING PARAMS FROM PARAM_TREE")
            reply = self._send_cmd('configure', self.config_tree.get('camera'))
        # logging.info(f"Configuration response: \n{self.format_json(reply)}")

    def _request_config(self):
        """Get the configuration of the PCO camera controller.

        This interal method sends a configuration request command to the PCO camera controller and
        displays the response as formatted JSON.
        """
        reply = self._send_cmd("request_configuration")
        return reply.attrs
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
