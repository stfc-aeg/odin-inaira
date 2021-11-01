
import logging
from operator import truediv
import os
import re
import sys
import threading
import queue
import click
import zmq


from .frame_producer import FrameProducer, FrameProducerConfig

from zmq.utils.strtypes import cast_bytes

from odin_data.ipc_channel import IpcChannel, IpcChannelException
from odin_data.ipc_message import IpcMessage, IpcMessageException

#TODO
# - Error Handling
# - State update when the rame producer has finished

class CameraEmulator:


    def __init__(self, endpoint):

        self.endpoint = endpoint

        # Create a logger and set level to debug
        self.logger = logging.getLogger(os.path.basename(sys.argv[0]))
        self.logger.setLevel(logging.DEBUG)

        # Create a console handler and set level to debug
        handler = logging.StreamHandler(sys.stdout)
        handler.setLevel(logging.DEBUG)

        # Create a log formatter
        formatter = logging.Formatter("%(asctime)s %(levelname)s %(name)s - %(message)s")

        # Add the formatter to the handler
        handler.setFormatter(formatter)

        # Add the handler to the logger
        self.logger.addHandler(handler)

        self.ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_ROUTER, self.endpoint)
        self._run_server = False

        self.frame_producer = FrameProducer("config/producer_test.yml")
        

        # State Maching variables
        self.state = 0
        self.states = ["disconnected",  # Client is not connected.                                  [0]
                        "connected",    # Client is connected                                       [1]
                        "armed",        # Client is connected. Camera Armed.                        [2]
                        "running"       # Client is connected. Camera Armed. Camera Running.        [3]
                        ]


    
        # Camera Info
        self.camera_info = {
                                "name" : "PCO Camera Emulator",
                                "acquisition" : {
                                                "acquiring" : False,
                                                "frames_acquired" : 0
                                                },
                                "camera" : {
                                            "state": "disconnected"
                                            },
                                "config" : {
                                            "number_of_frames" : 0,
                                            "delay_time_ms" : 1000,
                                            "exposure_time_ms" : 1000,
                                            "images_file_path" : "/aeg_sw/work/projects/inaira/odin-inaira/data/Test-Images/"
                                            }
                            }

    def run(self):

        self._run_server = True

        self.logger.info(f"Binding control IPC channel to endpoint {self.endpoint}")
        self.ctrl_channel.bind()


        # Handle keboard interrupt
        try:

            """Run the server socket task loop."""
            while self._run_server:

                (client_id, request) = self.ctrl_channel.recv()

                self.logger.info(f"Received request from client ID {client_id}: {request[0]}")
                ctrl_request = IpcMessage(from_str=request[0])

                ctrl_request_type = ctrl_request.get_msg_type()
                ctrl_request_val = str(ctrl_request.get_msg_val())
                ctrl_request_id = ctrl_request.get_msg_id()
                ctrl_request_command = ctrl_request.get_params().get("command")
                ctrl_request_ok = True

                if ctrl_request_type == 'cmd':

                    if ctrl_request_val == 'status':
                        self.logger.info(f"Got get status request from client {client_id}")
                    elif ctrl_request_val == "request_configuration":
                        self.logger.info(f"Got get configuration request from client {client_id}")
                    elif ctrl_request_val == "configure":
                        self.logger.info(f"Got instruction request from client {client_id}")
                    else:
                        # Unrecognised command value in request
                        ctrl_request_ok = False

                else:
                    # Unrecognised request type
                    ctrl_request_ok = False

                # Handle the command request
                if ctrl_request_ok:
                    ctrl_request_ok = state_machine(self, ctrl_request_command, client_id, ctrl_request, ctrl_request_val)
                    self.camera_info["camera"]["state"] = self.states[self.state]


                # Build a response
                ctrl_response_type = 'ack' if ctrl_request_ok else 'nack'
                ctrl_response = IpcMessage(
                    msg_type=ctrl_response_type, msg_val=ctrl_request_val, id=ctrl_request_id
                )

                # If there is no get request no params need to be sent.
                if ctrl_request_val == "status":
                    # Set the params for the status
                    self.camera_info["acquisition"]["frames_acquired"] = self.frame_producer.frame
                    ctrl_response.set_param("name", self.camera_info["name"])
                    ctrl_response.set_param("acquisition", self.camera_info["acquisition"])
                    ctrl_response.set_param("camera", self.camera_info["camera"])    


                if ctrl_request_val == "request_configuration":
                    
                    # Set the params for the config
                    self.logger.info("Returning camera config")
                    ctrl_response.set_param("enabled_packet_logging", False)
                    ctrl_response.set_param("frame_timeout_ms", 10)
                    ctrl_response.set_param("camera", self.camera_info["config"])

                # Encode and return the response to the client
                response = [elem.encode('utf-8') for elem in (client_id, ctrl_response.encode())]
                self.ctrl_channel.send_multipart(response)

        except KeyboardInterrupt:
            shut_down_producer(self)

        except Exception:
            shut_down_producer(self)
            self.logger.error("Exception in camera emulator")
                    
    # TODO Error handling in state machine
    def state_machine(self, ctrl_request_command, client_id, ctrl_request, ctrl_request_val):
        # State Machine
        # Can only go +- 1 state. NO JUMPS

        # Camera must be connected before status or config can be requested or sent

        # Connect to the camera
        if (ctrl_request_command == "connect") and (self.state == 0):
            self.state = 1
            self.logger.info(f"Client: {client_id} has connected to the camera")
            return True
            

        # Disconnect from the camera
        elif (ctrl_request_command == "disconnect") and (self.state == 1):
            self.state = 0
            self.logger.info(f"Client: {client_id} has disconnected from the camera")
            return True

        # Arm the camera
        elif (ctrl_request_command == "arm") and (self.state == 1):
            self.state = 2
            self.logger.info(f"Client: {client_id} has armed the camera")
            return True

        # Disarm the Camera
        elif (ctrl_request_command == "disarm") and (self.state == 2):
            self.state = 1
            self.logger.info(f"Client: {client_id} has disarmed the camera")
            return True

        # Start the camera
        elif (ctrl_request_command == "start") and (self.state == 2):

            # Start the camera
            delay = self.camera_info["config"]["delay_time_ms"]
            exposure = self.camera_info["config"]["exposure_time_ms"]
            fps = (delay + exposure)/1000
            self.frame_producer_thread = threading.Thread(target=self.frame_producer.run,
                                                        args=(self.camera_info["config"]["number_of_frames"],
                                                                fps,
                                                                self.camera_info["config"]["images_file_path"],
                                                                self)
                                                        )
            self.frame_producer_thread.start()
            self.state = 3
            self.camera_info["acquisition"]["acquiring"] = True
            self.logger.info(f"Client: {client_id} has started the camera")
            return True

        # Stop the camera
        elif (ctrl_request_command == "stop") and (self.state == 3):

            if self.frame_producer_thread.is_alive():
                # Stop the camera
                self.frame_producer.send_frames = False

            self.state = 2
            self.camera_info["acquisition"]["acquiring"] = False
            self.logger.info(f"Client: {client_id} has stopped the camera")
            return True

        elif ("camera" in ctrl_request.get_params()):
            request_config = ctrl_request.get_params().get("camera")
            self.camera_info["config"]["number_of_frames"] = request_config.get("num_frames", self.camera_info["config"]["number_of_frames"])
            self.camera_info["config"]["delay_time_ms"] = request_config.get("frame_delay", self.camera_info["config"]["delay_time_ms"])
            self.camera_info["config"]["exposure_time_ms"] = request_config.get("frame_exposure",self.camera_info["config"]["exposure_time_ms"])
            self.camera_info["config"]["images_file_path"] = request_config.get("images_path", self.camera_info["config"]["images_file_path"])
            return True

        elif ctrl_request_val == "status" or ctrl_request_val == "request_configuration":
            return True

        else:
            self.logger.warn("Incorrect command or request value.")
            return False

    def shut_down_producer(self):
    # Stop the frame producer if it is running
        if self.state == 3:
            self.frame_producer_thread.join()
            self.frame_producer.send_frames = False

        # Shut down the process release and clear memory buffer
        self.frame_producer_stop_thread = threading.Thread(target=self.frame_producer.stop)
        self.frame_producer_stop_thread.start()
        self.frame_producer_stop_thread.join()                     





@click.command()
@click.option("--ctrl", default="tcp://127.0.0.1:5061", show_default=True,
    help="Camera control channel endpoint URL", metavar="URL")
def main(ctrl):

    emulator = CameraEmulator(ctrl)
    emulator.run()

if __name__ == '__main__':
    main()