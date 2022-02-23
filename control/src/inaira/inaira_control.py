import logging
import os
import sys
import time


from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError
from tornado.ioloop import PeriodicCallback

from .odin_inaira import OdinInaira, OdinInairaError
from .camera_controller import CameraController

# TODO 
#   Add clipping back into live view
#   Improve status controls
#   Imporve UI
#   Probably someting else ngl

class InairaControl():
    
    def __init__(self, endpoints, live_image, ctrl_endpoint, status_refresh):

        self.odin_inaira = OdinInaira(endpoints, live_image)
        self.camera_controller = CameraController(ctrl_endpoint, status_refresh)
        self.logger = logging.getLogger(os.path.basename(sys.argv[0]))
        self.logger.setLevel(logging.DEBUG)

        # Store initialisation time
        self.init_time = time.time()

        self.param_tree = ParameterTree({
            'odin_inaira' : self.odin_inaira.param_tree,
            'camera_control' : self.camera_controller.param_tree,
        })


        # Start the auto update of the camera controller
        self.background_update = PeriodicCallback(self.camera_controller.get_camera_state, status_refresh)
        self.background_update.start()
        

    def get(self, path):
        return self.param_tree.get(path)

    def set(self, path, data):
        return self.param_tree.set(path, data)

    def get_server_uptime(self):
        return time.time() - self.init_time

    def cleanup(self):
        self.odin_inaira.cleanup()
        # TODO Make cleanup for camera controller

class InairaControlError(Exception):

    pass