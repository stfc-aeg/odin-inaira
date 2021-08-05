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
import numpy as np

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
        """Initialise the OdinInaira object.        # Roll past 100 array to 1 to the right ->
        self.past_100 = np.roll(self.past_100, 1, axis=0)

        # Set replace first item in past 100
        self.past_100[0] = [self.frame_number, self.frame_classification, self.frame_certainty]

        This constructor initlialises the Workshop object, building a parameter tree and
        launching a background task if enabled
        """

        logging.debug("Inistialising INAIRA Adapter")

        # Save and initialise argumenets
        self.check_counter = 0
        self.frame_number = None
        self.frame_process_time = None

        self.test_array = [None]*100
        self.frame_data = None

        # Store initialisation time
        self.init_time = time.time()

        # Get package version information
        version_info = get_versions()

        # Build a parameter tree for the frame data
        # frame_data_parameter = ParameterTree({
        #     'frame_number': (lambda: self.frame_number, None),
        #     'frame_process_time': (lambda: self.frame_process_time, None),
        #     'frame_classification': (lambda: self.frame_classification, None),
        #     'frame_certainty': (lambda: self.frame_certainty, None)
        # })

        # Store all information in a parameter tree
        self.param_tree = ParameterTree({
            'odin_version': version_info['version'],
            'tornado_version': tornado.version,
            'server_uptime': (self.get_server_uptime, None),
            'frame_data' : (lambda: self.frame_data, None)
        })

        logging.debug('Parameter tree initialised')

        # Subscribe to INAIRA Odin Data Adapter
        self.ipc_channels = []
        for endpoint in endpoints:
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


    def get_server_uptime(self):
        return time.time() - self.init_time

    def get(self, path):
        return self.param_tree.get(path)

    def get_frame_updates(self, msg):

        self.frame_data = json.loads(msg[0])
        logging.debug(self.frame_data)

        np.roll(self.test_array, 1)
        self.test_array[0] = self.frame_data


    def cleanup(self):
        for channel in self.ipc_channels:
            channel.cleanup()

class OdinInairaError(Exception):

    pass

