
import logging
import os
import sys
import threading
import queue
import click
import zmq


from .frame_producer import FrameProducer, FrameProducerConfig

from zmq.utils.strtypes import cast_bytes

from odin_data.ipc_channel import IpcChannel, IpcChannelException
from odin_data.ipc_message import IpcMessage, IpcMessageException


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

        # Placeholder config storage
        self.armed = False
        self.connected = False
        self.running = False
        self.response_message = None

        # Config storage
        self.frames = None
        self.fps = None
        self.imgs_path = None

    def run(self):

        self._run_server = True

        self.logger.info(f"Binding control IPC channel to endpoint {self.endpoint}")
        self.ctrl_channel.bind()

        """Run the server socket task loop."""
        while self._run_server:

            (client_id, request) = self.ctrl_channel.recv()

            self.logger.info(f"Received request from client ID {client_id}: {request[0]}")
            ctrl_request = IpcMessage(from_str=request[0])

            ctrl_request_type = ctrl_request.get_msg_type()
            ctrl_request_val = ctrl_request.get_msg_val()
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

            # Hande the command request
            if ctrl_request_ok:

                self.response_message = "No response given"

                # Camera must be connected to before config can be sent or status can be requested
                # Camera must be configured before it can be armed
                # Camera must be connected too and armed before it can be started
                
                if ctrl_request_command == "status":
                    self.logger.info(f"Client: {client_id} has requested camera status")
                    self.response_message = "Camera status:"

                if ctrl_request_command == "config":
                    # Get the data out of the JSON Config message
                    config = ctrl_request.get_param("camera")

                    self.frames = config["frames"]
                    self.fps = config["frame_rate"]
                    self.imgs_path = config["images_path"]

                    self.response_message = f"Client: {client_id} has sent configuration to camera"

                # Act Upon Request
                elif ctrl_request_command == "connect":
                    # Emulate connecting to the camera
                    if self.connected:
                        self.logger.warning(f"Client: {client_id} is already connected to camera")
                        self.response_message = "Client is already connected to camera"
                    else:
                        self.connected = True
                        self.logger.info(f"Client: {client_id} connected to the camera")
                        self.response_message = "Client has connected to the camera"

                elif ctrl_request_command == "disconnect":
                    # Emulate disconnecting from the camera
                    if self.connected:
                        self.connected = False
                        self.logger.info(f"Client: {client_id} has disconnected from the camera")
                        self.response_message = "Client has disconnected from the camera"
                    else:
                        self.logger.warning(f"Client: {client_id} is already disconnected from the camera")
                        self.response_message = "Client is already disconnected from the camera"
                        

                elif ctrl_request_command == "arm":
                    if self.connected:
                        if self.armed:
                            self.logger.warning("The camera is already armed")
                            self.response_message = "Client has already armed the camera"
                        else:
                            self.armed = True
                            self.logger.info(f"Client: {client_id} has armed the camera")
                            self.response_message = "Client has armed the camera"
                    else:
                        self.logger.warning(f"Client: {client_id} is not connected to the camera")
                        self.response_message = "Client is not connected to the camera"

                elif ctrl_request_command == "disarm":
                    if self.connected:
                        if self.armed:
                            self.armed = False
                            self.logger.info(f"Client: {client_id} has disarmed the camera")
                            self.response_message = "Client has disarmed the camera"
                        else:
                            self.logger.warning("The camera is already disarmed")
                            self.response_message = "The camera is not armed"
                    else:
                        self.logger.warning(f"Client: {client_id} is not connected to the camera")
                        self.response_message = "Client is not connected to the camera"

                elif ctrl_request_command == "start":
                    if self.connected & self.armed:
                        if self.running:
                            self.logger.warning(f"Client: {client_id} has already started the camera")
                            self.response_message = "Client has already started the camera"
                        else:
                            self.logger.info("Starting Frame Producer")
                            try:
                                self.frame_producer.send_frames = True
                                self.frame_producer_thread = threading.Thread(target=self.frame_producer.run, args=(self.frames, self.fps, self.imgs_path))
                                self.frame_producer_thread.start()
                            except:
                                self.logger.error("Error running frame producer")
                            self.running = True
                            self.response_message = "Client has started the camera"
                    else:
                        self.logger.warning(f"Client: {client_id} is not connected to, or has not armed the camera")
                        self.response_message = "The camera is not armed or the client is not connected."

                elif ctrl_request_command == "stop":
                    if self.connected & self.armed:
                        if self.running:
                            self.logger.info("Stopping frame producer")
                            self.frame_producer.send_frames = False
                            self.frame_producer_thread.join()
                            self.running = False
                            self.response_message = "Client has stopped the camera"
                        else:
                            self.logger.warning("Client has already stopped the camera")
                    else:
                        self.logger.warning(f" Client: {client_id} is not connected to, or has not armed the camera")
                        self.response_message = "The camera is not armed of the client is not connected"
            else:
                self.logger.warning(f"Client: {client_id} has made an invalid control request")
                self.response_message = "The client has made an invalid control request"
            
            # Build the response message
            ctrl_response_val = ('{"reponse" : "%s", "camera status": { "connected" : "%s", "armed" : "%s", "running" : "%s"} }'%(self.response_message,
                                                                                                    str(self.connected),
                                                                                                    str(self.armed),   
                                                                                                    str(self.running)))

            # Build a response
            ctrl_response_type = 'ack' if ctrl_request_ok else 'nack'
            ctrl_response = IpcMessage(
                msg_type=ctrl_response_type, msg_val=ctrl_request_val, id=ctrl_request_id
            )
            ctrl_response.set_param("state", 1)

            # Encode and return the response to the client
            response = [elem.encode('utf-8') for elem in (client_id, ctrl_response.encode())]
            self.ctrl_channel.send_multipart(response)

@click.command()
@click.option("--ctrl", default="tcp://127.0.0.1:5061", show_default=True,
    help="Camera control channel endpoint URL", metavar="URL")
def main(ctrl):

    emulator = CameraEmulator(ctrl)
    emulator.run()

if __name__ == '__main__':
    main()