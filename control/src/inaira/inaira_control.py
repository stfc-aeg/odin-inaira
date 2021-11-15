import logging
import os
import sys


from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError
from tornado.ioloop import PeriodicCallback

from .odin_inaira import OdinInaira, OdinInairaError
from .camera_controller import CameraController

# TODO 
#   Test with recieving frames..
#   Link to camera controller..

class InairaControl():
    
    def __init__(self, endpoints, live_image, ctrl_endpoint, status_refresh):

        self.odin_inaira = OdinInaira(endpoints, live_image)
        self.camera_controller = CameraController(ctrl_endpoint, status_refresh)
        self.logger = logging.getLogger(os.path.basename(sys.argv[0]))
        self.logger.setLevel(logging.DEBUG)

        self.frame_rate = 12

        self.param_tree = ParameterTree({
            'odin_inaira' : self.odin_inaira.param_tree,
            'camera_control' : self.camera_controller.param_tree,
            'frame_rate' : (lambda: self.frame_rate, lambda a: self.logger.info("You set the frame rate... also whever this is: " + str(a)))
        })


        # Start the auto update of the camera controller
        self.background_update = PeriodicCallback(self.camera_controller.get_camera_state, status_refresh)
        self.background_update.start()
        

    def get(self, path):
        return self.param_tree.get(path)

    def set(self, path, data):
        return self.param_tree.set(path, data)

    def cleanup(self):
        self.odin_inaira.cleanup()
        # TODO Make cleanup for camera controller

class InairaControlError(Exception):

    pass