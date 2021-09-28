# Control script for the INAIRA project PCO camera integration
#
# This script communicates with the PCO camera control integrated into the odin-data frame
# receiver as part of the INAIRA project. The communication takes place over a ZeroMQ IPC 
# channel. The script uses the click package to implmenent a command-line interface supporting
# multiple commands.
#
# Tim Nicholls, STFC Detector Systems Software Group
import logging
import sys
import os
import pprint
import time
import argparse
import json
try:
    from json.decoder import JSONDecodeError
except ImportError:
    JSONDecodeError = ValueError

try:
    from pygments import highlight
    from pygments.lexers import JsonLexer
    from pygments.formatters import TerminalFormatter
    has_pygments = True
except ImportError:
    has_pygments = False

import click

from odin_data.ipc_channel import IpcChannel, IpcChannelException
from odin_data.ipc_message import IpcMessage, IpcMessageException

class CameraController():
    """PCO camera control utility.
    
    This class implements the PCO camera control functionality for the INAIRA project, 
    communicating with the PCO controller integrated into an odin-data frame receiver decoder
    plugin via a ZeroMQ IPC channel.
    """
    # Define the maximum message ID for the IPC channel
    MESSAGE_ID_MAX = 2**32

    def __init__(self, ctrl_endpoint):
        """Initialise the CameraController object.

        This method initialises the controller object, setting up the IPC control channel and
        creating a message logger.

        :param ctrl_endpoint: ZeroMQ URL for the camera control endpoint
        """
        # Set the control endpoint to the specified value
        self.ctrl_endpoint = ctrl_endpoint

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

        # Create an IPC channel to talk to the camera control endpoint
        self.ctrl_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_DEALER)

        # Connect to the camera control endpoint
        self.ctrl_channel.connect(self.ctrl_endpoint)
        self.logger.info(f"Controller connected to camera at endpoint: {self.ctrl_endpoint}")

        # Initialise the message id counter and timeout
        self._msg_id = 0
        self._timeout_ms = 1000

        # Create a pretty printer object if not using pygments to format JSON output
        if not has_pygments:
            self._pp = pprint.PrettyPrinter()

    def _next_msg_id(self):
        """Return the next message ID for the control channel.
        
        This internal method increments and returns the next value of the message ID sent on the
        control IPC channel.
        """
        self._msg_id = (self._msg_id + 1) % self.MESSAGE_ID_MAX
        return self._msg_id

    def send_command(self, command):
        """Send a control command to the PCO camera controller.
        
        This method sends a control command to the PCO camera. This is implemented as an IPC
        configuration message with the appropriate camera command in the payload.
        
        :param command: string camera control command
        """
        params = {'command': command}
        self._send_configure(params)

    def _send_configure(self, params=None):
        """Send a configuration command to the PCO camera controller.
        
        This internal method sends a configuration command to the PCO camera controller, with
        the specified parameter dict as a payload.
        
        :param params: dictionary of parameters to add to the IPC channel message.
        """
        reply = self._send_cmd("configure", params)
        self.logger.info(f"Got reply: {reply}")

    def get_status(self):
        """Get the status of the PCO camera controller.
        
        This method send a status request command to the PCO camera controller and displays
        the response as formatted JSON.
        """
        reply = self._send_cmd("status")
        self.logger.info(f"Status response: \n{self.format_json(reply)}")

    def _send_cmd(self, cmd, params=None):
        """Send an IPC command message to the PCO camera controller.
        
        This internal method sends an IPC command message to the PCO camera controller, with the
        specified parameter dict as the payload.

        :param cmd: string command 
        """
        cmd_msg = IpcMessage("cmd", cmd, id=self._next_msg_id())
        if params:
            cmd_msg.attrs["params"] = params

        self.ctrl_channel.send(cmd_msg.encode())

        reply = None
        pollevts = self.ctrl_channel.poll(self._timeout_ms)
        if pollevts == IpcChannel.POLLIN:
            reply = self.ctrl_channel.recv()

        return IpcMessage(from_str=reply)

    def format_json(self, msg):
        """Format JSON data for display.
        
        This method formats JSON-like dict data for display. The data can be supplied directly or
        passed as an IPC message. If the pygments package is installed, that will be used to 
        generate a colour, formatted output, otherwise the default PrettyPrinter will be used.

        :param msg: JSON-like dict or IPC message to display
        """
        if isinstance(msg, IpcMessage):
            json_data = msg.attrs
        else:
            json_data = msg

        if has_pygments:
            json_str = json.dumps(json_data, indent=2, sort_keys=False)
            formatted = highlight(json_str, JsonLexer(), TerminalFormatter())
        else:
            formatted = self._pp.pformat(json_data)

        return formatted


@click.group()
@click.option('--ctrl', default="tcp://127.0.0.1:5060", help="Camera control channel endpoint URL")
@click.pass_context
def cli(ctx, ctrl):
    """Control a PCO camera integrated into the odin-data frame receiver.
    \f
    :param ctx: click context
    :param ctrl: ZeroMQ endpoint URL for the control channel
    """
    ctx.obj['controller'] = CameraController(ctrl)

@cli.command()
@click.pass_context
def connect(ctx):
    """Connect to the PCO camera.\f
    
    :param ctx: click context
    """
    ctx.obj['controller'].send_command("connect")

@cli.command()
@click.pass_context
def disconnect(ctx):
    """Disconnect from the PCO camera.\f
    
    :param ctx: click context
    """
    ctx.obj['controller'].send_command("disconnect")

@cli.command()
@click.pass_context
def arm(ctx):
    """Arm the PCO camera.\f
    
    :param ctx: click context
    """
    ctx.obj['controller'].send_command("arm")

@cli.command()
@click.pass_context
def disarm(ctx):
    """Disarm the PCO camera.\f
    
    :param ctx: click context
    """
    ctx.obj['controller'].send_command("disarm")

@cli.command()
@click.pass_context
def start(ctx):
    """Start frame acquisition on the PCO camera.\f
    
    :param ctx: click context
    """
    ctx.obj['controller'].send_command("start")

@cli.command()
@click.pass_context
def stop(ctx):
    """Stop frame acquisition on the PCO camera.\f
    
    :param ctx: click context
    """
    ctx.obj['controller'].send_command("stop")

@cli.command()
@click.pass_context
def status(ctx):
    """Get the status of the PCO camera.\f
    
    :param ctx: click context
    """
    ctx.obj['controller'].get_status()

def main():
    """Main entry point for the camera control utility."""
    cli(obj={})

if __name__ == '__main__':
    main()