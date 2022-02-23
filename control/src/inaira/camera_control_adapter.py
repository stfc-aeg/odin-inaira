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

from .camera_controller import CameraController


class CameraControlAdapter(ApiAdapter):

    def __init__(self, **kwargs):
        super(CameraControlAdapter, self).__init__(**kwargs)

        ctrl_endpoint = self.options.get("ctrl_endpoint", "")
        status_refresh = self.options.get("status_loop_time", 1000)
        self.controller = CameraController(ctrl_endpoint, status_refresh)

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

