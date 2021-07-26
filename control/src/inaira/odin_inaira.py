"""
Adapter for ODIN INAIRA

This class 'fill me in'

David Symons, Some random apprentice Tim found
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

class OdinInaira():

    executor = futures.ThreadPoolExecutor(max_workers=1)

    def __init__(self, get_frame_updates, connection_address):
        """Initialise the OdinInaira object.

        This constructor initlialises the Workshop object, building a parameter tree and
        launching a background task if enabled
        """
        # Save argumenets
        self.check_counter = 0
        self.frame_updates = get_frame_updates
        self.connection_string = connection_address

        # Store initialisation time
        self.init_time = time.time()

        # Get package version information
        version_info = get_versions()

        # Build a parameter tree for the frame data
        frame_data = ParameterTree({
            'frame_number': (lambda: self.frame_number, None),
            'frame_process_time': (lambda: self.frame_process_time, None),
            'frame_results': (lambda: self.frame_results, None),
            'test_counter': (lambda: self.check_counter, None)
        })

        # Store all information in a parameter tree
        self.param_tree = ParameterTree({
            'odin_version': version_info['version'],
            'tornado_version': tornado.version,
            'server_uptime': (self.get_server_uptime, None),
            'frame': frame_data 
        })

        self.run_check_counter()
        self.get_frame_updates()

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

    def get_frame_updates(self):

        # Create connection to INAIRA MI on ODIN DATA
        context = zmq.Context()
        socket = context.socket(zmq.SUB)
        socket.connect(self.connection_string)
        socket.subscribe() # TODO Filter if necessary
        logging.debug("Listening to .. add rest of connection string")

        # Get the frame data
        while self.frame_updates:
            frame_data = json.loads(socket.recv())
            # TODO manipulate json into tree

class OdinInairaError(Exception):
    
    pass

