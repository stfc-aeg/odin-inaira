"""
Adapter for ODIN INAIRA

This class 'fill me in'

David Symons
"""

import logging
import tornado
import time
import sys
import zmq
import json

from concurrent import futures

from tornado.ioloop import IOLoop, PeriodicCallback
from tornado.concurrent import run_on_executor
from tornado.escape import json_decode

from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, request_types, response_types
from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError
from odin._version import get_versions

from odin_data.ipc_channel import IpcChannelException

from .sub_socket import SubSocket

# TODO Another in line: 58

class OdinInaira(object):

    executor = futures.ThreadPoolExecutor(max_workers=1)

    def __init__(self, endpoints):
        """Initialise the OdinInaira object.

        This constructor initlialises the Workshop object, building a parameter tree and
        launching a background task if enabled
        """

        logging.debug("Inistialising INAIRA Adapter")

        # Save and initialise argumenets
        self.check_counter = 0
        self.frame_number = None
        self.frame_process_time = None
        self.frame_result_a = None
        self.frame_result_b = None
        # Store initialisation time
        self.init_time = time.time()

        # Get package version information
        version_info = get_versions()

        # Build a parameter tree for the frame data
        # TODO Allow results to be handled as an array
        frame_data_parameter = ParameterTree({
            'frame_number': (lambda: self.frame_number, None),
            'frame_process_time': (lambda: self.frame_process_time, None),
            'frame_result': (lambda: self.frame_result_a, None),
            'test_counter': (lambda: self.check_counter, None)
        })

        # Store all information in a parameter tree
        self.param_tree = ParameterTree({
            'odin_version': version_info['version'],
            'tornado_version': tornado.version,
            'server_uptime': (self.get_server_uptime, None),
            'frame': frame_data_parameter 
        })

        logging.debug('Parameter tree initialised')

        # Subscribe to INAIRA Odin Data Adapter
        self.endpoints = endpoints
        self.ipc_channels = []
        for endpoint in self.endpoints:
            try:
                tmp_channel = SubSocket(self, endpoint)
                self.ipc_channels.append(tmp_channel)
                logging.debug("Subscribed to endpoint: %s", tmp_channel.endpoint)
            except IpcChannelException as chan_error:
                logging.warning("Unable to subscribe to %s: %s", endpoint, chan_error)

        logging.debug("Connected to %d endpoints", len(self.ipc_channels))

        if not self.ipc_channels:
            logging.warning(
                "Warning: No subscriptions made. Check the configuration file for valid endpoints")

        self.run_check_counter()

    def get_server_uptime(self):
        return time.time() - self.init_time

    def get(self, path):
        return self.param_tree.get(path)

    def set(self, path, data):
        try:
            self.param_tree.set(path, data)
        except ParameterTreeError as e:
            raise OdinInairaError(e)

    @run_on_executor
    def run_check_counter(self):
        while self.check_counter < 100:
            time.sleep(1)
            self.check_counter += 1

    def get_frame_updates(self, msg):

        frame_data = json.loads(msg[0])
        logging.debug(frame_data)

        # Set Frame Number
        self.frame_number = frame_data['frame_number'] #int
        # Set Frame Process Time
        self.frame_process_time = frame_data['process_time'] #float
        # Set Frame Results
        self.frame_result = frame_data['result'] #float list

    def cleanup(self):
        for channel in self.ipc_channels:
            channel.cleanup()

class OdinInairaError(Exception):
    
    pass

