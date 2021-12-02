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
import json
import re
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

    def do_command(self, command):
        """Send a control command to the PCO camera controller.

        This method sends a control command to the PCO camera. This is implemented as an IPC
        configuration message with the appropriate camera command in the payload.

        :param command: string camera control command
        """
        params = {"command": command}
        reply = self._send_cmd("configure", params)
        self.logger.info(f"Command response: \n{self.format_json(reply)}")


    def do_config(self, json_path=None, json_vals=None):
        """Send or request configuration parameters to/from the PCO camera controller.

        This method sends or requests configuration parameters to/from the PCO camera controller.
        These can either by provided in a JSON file at the specified path, which will be loaded and
        injected into the configuration message, or as individual key-value pairs. If neither a file
        nor any key-value pairs are specified, request the current configuration instead

        :param json_path: optional JSON config file path to load
        :param json_vals: tuple of key-value pairs to parse into JSON fields
        """

        # Create the appropriate parameter payload to send to the controller
        params = {"camera": {}}

        # If a json file path has been specified, attempt to load that file
        if json_path:
            try:
                with open(json_path) as json_file:
                    params["camera"] = json.load(json_file)
            except JSONDecodeError as json_error:
                self.logger.error(f"Failed to decode JSON config file: {json_error}")
                return

        # If JSON arguments were specified, update the parameter payload with those values
        if json_vals:
            params["camera"].update(json_vals)

        # If parameters have been specified, send the configuration command message to the camera
        # controller, otherwise print a warning
        if params["camera"]:
            self._send_config(params)
        else:
            self._request_config()
#            self.logger.warning("No camera configuration parameters specified, not sending command")

    def _send_config(self, params=None):
        """Set PCO camera controller confiugration parameters.

        This internal method sends a configuration command to the PCO camera controller, with
        the specified parameter dict as a payload.

        :param params: dictionary of parameters to add to the IPC channel message.
        """
        response = self._send_cmd("configure", params)
        if response:
            self.logger.info(f"Configuration response: \n{self.format_json(response)}")
        else:
            self.logger.error("Timeout waiting for response to configuration set request")


    def _request_config(self):
        """Get the configuration of the PCO camera controller.

        This interal method sends a configuration request command to the PCO camera controller and
        displays the response as formatted JSON.
        """
        response = self._send_cmd("request_configuration")
        if response:
            self.logger.info(f"Config request response: \n{self.format_json(response)}")
        else:
            self.logger.error("Timeout waiting for response to configuration get request")

    def get_status(self):
        """Get the status of the PCO camera controller.

        This method send a status request command to the PCO camera controller and displays
        the response as formatted JSON.
        """
        response = self._send_cmd("status")
        if response:
            self.logger.info(f"Status response: \n{self.format_json(response)}")
        else:
            self.logger.error("Timeout waiting for response to status request")

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

        response = None
        pollevts = self.ctrl_channel.poll(self._timeout_ms)
        if pollevts == IpcChannel.POLLIN:
            response_msg = self.ctrl_channel.recv()
            response = IpcMessage(from_str=response_msg)

        return response

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


def parse_json_args(ctx, _, values):
    """Parse argument key-value pairs into JSON-like parameters.

    This function is used as a click argument callback to parse arguments to the
    config command from key-value pairs with JSON-like syntax into a dict of
    configuration parameters. The key-value parsing supports simple string parameters
    or the HTTPie-like := assignment of raw JSON types.

    :param ctx: command execution context
    :param _: ignored (click passes in the parameter object)
    :param values: tuple of argument values specified at the command line
    :return: dict of parsed values
    """
    # Define a mapping of key-value separators to value parsing
    keyval_parsers = {
        '=': lambda val: val,
        ':=': lambda val: json.loads(val)
    }

    # Complile a regular expression to allow input values to be split on the appropriae
    # key-value separators
    parse_exp = re.compile('(' + '|'.join(keyval_parsers.keys()) + ')')

    parsed_args = {}

    # Iterator over the argument values, splitting them into key, separator and value and
    # parsing as appropriate. Append parsed valyes to the parsed_args dict.
    for json_val in values:
        try:
            (key, sep, val) = re.split(parse_exp, json_val)
            parsed_args[key] = keyval_parsers[sep](val)
        except ValueError:
            ctx.obj["controller"].logger.warning(
                f"Ignoring incorrectly formatted parameter argument {json_val}"
            )

    return parsed_args

@click.group()
@click.option("--ctrl", default="tcp://127.0.0.1:5060", show_default=True,
    help="Camera control channel endpoint URL", metavar="URL")
@click.pass_context
def cli(ctx, ctrl):
    """Control a PCO camera integrated into the odin-data frame receiver.
    \f
    :param ctx: command execution context
    :param ctrl: ZeroMQ endpoint URL for the control channel
    """
    ctx.obj["controller"] = CameraController(ctrl)

@cli.command()
@click.pass_context
def connect(ctx):
    """Connect to the PCO camera.\f

    :param ctx: command execution context
    """
    ctx.obj["controller"].do_command("connect")

@cli.command()
@click.pass_context
def disconnect(ctx):
    """Disconnect from the PCO camera.\f

    :param ctx: command execution context
    """
    ctx.obj["controller"].do_command("disconnect")

@cli.command()
@click.pass_context
def arm(ctx):
    """Arm the PCO camera.\f

    :param ctx: command execution context
    """
    ctx.obj["controller"].do_command("arm")

@cli.command()
@click.pass_context
def disarm(ctx):
    """Disarm the PCO camera.\f

    :param ctx: command execution context
    """
    ctx.obj["controller"].do_command("disarm")

@cli.command()
@click.pass_context
def start(ctx):
    """Start frame acquisition on the PCO camera.\f

    :param ctx: command execution context
    """
    ctx.obj["controller"].do_command("start")

@cli.command()
@click.pass_context
def stop(ctx):
    """Stop frame acquisition on the PCO camera.\f

    :param ctx: command execution context
    """
    ctx.obj["controller"].do_command("stop")

@cli.command()
@click.pass_context
def reset(ctx):
    """Reset error condition on the PCO camera controller.\f

    :param ctx: command execution context
    """
    ctx.obj["controller"].do_command("reset")

@cli.command()
@click.option("--json", "-j", type=click.Path(exists=True),
    help="Specify a JSON file of camera configuration parameters to send")
@click.argument('vals', nargs=-1, callback=parse_json_args)
@click.pass_context
def config(ctx, json, vals):
    """Set or get configuration parameters to/from the PCO camera.

    Parameters can be loaded from the specified JSON file with the --json/-j option,
    or specified as arguments in key-value pair form. The HTTPie syntax for JSON is
    supported: key=value will be treated as string parameters, key:=value will be
    intepreted as 'raw' JSON values, e.g. numbers, booleans, arrays or objects.

    If no JSON file or key-value pairs are specified, request the current configuration
    instead.
    \f

    :param ctx: command execution context
    :param vals: tuple of key-value pairs
    """
    ctx.obj["controller"].do_config(json_path=json, json_vals=vals)

@cli.command()
@click.pass_context
def status(ctx):
    """Get the status of the PCO camera controller.\f

    :param ctx: command execution context
    """
    ctx.obj["controller"].get_status()

def main():
    """Main entry point for the camera control utility."""
    cli(obj={})

if __name__ == "__main__":
    main()