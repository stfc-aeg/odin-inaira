
import logging
import os
import sys

import click
import zmq
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
            ctrl_request_ok = True

            if ctrl_request_type == 'cmd':

                if ctrl_request_val == 'status':
                    self.logger.info(f"Got status request from client {client_id}")
                else:
                    # Unrecognised command value in request
                    ctrl_request_ok = False

            else:
                # Unrecognised request type
                ctrl_request_ok = False

            # Build a response
            ctrl_response_type = 'ack' if ctrl_request_ok else 'nack'
            ctrl_response = IpcMessage(
                msg_type=ctrl_response_type, msg_val=ctrl_request_val, id=ctrl_request_id
            )

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