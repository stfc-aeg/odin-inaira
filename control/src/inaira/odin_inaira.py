"""
Adapter for ODIN INAIRA

This class controls communication between Odin Control and Odin Data when the INAIRA
plugin is in use. 

The live view adapter is used to display images.
The INAIRA adapter controls and displays frame data. 

David Symons
"""

import logging
from threading import currentThread
import tornado
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


class OdinInaira(object):

    executor = futures.ThreadPoolExecutor(max_workers=1)

    def __init__(self, endpoints, live_image):
        """
        Initialise the OdinInaira object.

        This constructor initlialises the OdinInaira object, building a parameter tree and
        launching a background task if enabled
        """

        logging.debug("Inistialising INAIRA Adapter")

        # Save and initialise argumenets
        self.frame_process_time = 10
        self.total_process_time = 11
        self.pass_ratio = 1
        self.good_frames = 1
        self.classification = "Good"
        self.certainty = 1
        self.frame_number = 1
        self.process_time = 1
        self.avg_process_time = 1
        self.total_frames = 1

        self.process_live_image = live_image

        # Get package version information
        version_info = get_versions()

        self.adapters = {}

        self.frame_data_parameters = ParameterTree({
            'frame_number' : (lambda: self.frame_number, None),
            'process_time' : (lambda: self.process_time, None),
            'classification' : (lambda: self.classification, None),
            'certainty' : (lambda: self.certainty, None)
        })

        self.experiment_data_parameters = ParameterTree({
            'avg_processing_time' : (lambda: self.avg_process_time, None),
            'pass_ratio' : (lambda: self.pass_ratio, None),
            'total_frames' : (lambda: self.total_frames, None)
        })

        # Store all information in a parameter tree
        self.param_tree = ParameterTree({
            'odin_version': version_info['version'],
            'tornado_version': tornado.version,
            'frame' : self.frame_data_parameters,
            'experiment' : self.experiment_data_parameters

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

    def get(self, path):
        return self.param_tree.get(path)

    def get_frame_updates(self, msg):
        frame_data = json.loads(msg[0])

        if self.process_live_image:
            logging.info("Sending Image data from Inaira to Live View")
            liveview_data = msg[1:]
            liveview_adapter = self.adapters['live_view']
            liveview_adapter.live_viewer.create_image_from_socket(liveview_data)

        self.frame_number = frame_data['frame_number']
        self.total_frames = self.frame_number + 1
        self.process_time = frame_data['process_time']

        #Do the maths for the pass_ratio
        if (self.frame_number < 1):
            self.good_frames = 0
            self.total_process_time = 0

        if(frame_data['result'][0] < frame_data['result'][1]):
            self.good_frames += 1
            self.classification = "Good"
            self.certainty = frame_data['result'][1]
        else:
            self.classification = "Bad"
            self.certainty = frame_data['result'][0]

        self.pass_ratio = self.good_frames/self.total_frames

        logging.debug(" Pass ratio = " + str(self.pass_ratio))

        #Do the maths for the avg_process_time

        self.total_process_time += self.process_time
        self.avg_process_time = self.total_process_time/self.total_frames

        logging.debug(" Avg Process Time = " + str(self.avg_process_time) + "ms")


        return False

    def cleanup(self):
        for channel in self.ipc_channels:
            channel.cleanup()

class OdinInairaError(Exception):

    pass

