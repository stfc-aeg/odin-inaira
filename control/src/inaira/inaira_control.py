import logging
import os
import sys
import time


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

        self.change = None
        self.state_up_button_text = "Connect"
        self.state_up_button_enabled = True
        self.state_down_button_text = "Disconnect"
        self.state_down_button_enabled = False

        # Store initialisation time
        self.init_time = time.time()

        self.status_change = ParameterTree({
            'change' : (lambda: self.change, self.update_state),
            'down_button_text' : (lambda: self.state_down_button_text, None),
            'down_button_enabled' : (lambda: self.state_down_button_enabled, None),
            'up_button_text' : (lambda: self.state_up_button_text, None),
            'up_button_enabled' : (lambda: self.state_up_button_enabled, None)
        })

        self.param_tree = ParameterTree({
            'odin_inaira' : self.odin_inaira.param_tree,
            'camera_control' : self.camera_controller.param_tree,
            'status_change' : self.status_change
        })


        # Start the auto update of the camera controller
        self.background_update = PeriodicCallback(self.camera_controller.get_camera_state, status_refresh)
        self.background_update.start()
        

    def get(self, path):
        return self.param_tree.get(path)

    def set(self, path, data):
        return self.param_tree.set(path, data)

    def update_state(self, state_change):
        # Set the button text as their defaults.
        up_text = self.state_up_button_text
        up_enabled = self.state_up_button_enabled
        down_text = self.state_down_button_text
        down_enabled = self.state_down_button_enabled

        command = None
        state = self.camera_controller.state
        self.logger.debug("The cameras state is:" + state)
        if state == "disconnected":
            if state_change == "up":
                command = "connect"
                up_text, down_text, up_enabled, down_enabled = "Arm", "Disconnect", True, True
            else:
                command = ""
                self.logger.warn("Incorrect State Change Request: A camera in the disconnected state cannot be disconnected further")
        elif state == "connected":
            if state_change == "up":
                command = "arm"
                up_text, down_text, up_enabled, down_enabled = "Start", "Disarm", True, True
            else:
                command = "disconnect"
                up_text, down_text, up_enabled, down_enabled = "Connect", "Disconnect", True, False
        elif state == "armed":
            if state_change == "up":
                command = "start"
                up_text, down_text, up_enabled, down_enabled = "Start", "Stop", False, True
            else:
                command = "disarm"
                up_text, down_text, up_enabled, down_enabled = "Start", "Disconnect", True, True
        elif state == "running":
            if state_change == "down":
                command = "stop"
                up_text, down_text, up_enabled, down_enabled = "Start", "Disarm", True, True
            else:
                command = ""
                self.logger.warn("Incorrect State Change Request: A camera in the running state cannot be told to run more")
        try:
            self.camera_controller.do_command(command)
        except:
            self.logger.warn("Error or something.. I need to improve the error handling :D")
            # Also if this fails then this should braek out of this here :D
        
        self.state_up_button_text = up_text
        self.state_up_button_enabled = up_enabled
        self.state_down_button_text = down_text
        self.state_down_button_enabled = down_enabled

    def get_server_uptime(self):
        return time.time() - self.init_time

    def cleanup(self):
        self.odin_inaira.cleanup()
        # TODO Make cleanup for camera controller

class InairaControlError(Exception):

    pass