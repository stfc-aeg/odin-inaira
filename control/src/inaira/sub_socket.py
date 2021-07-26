"""
Adapter for ODIN INAIRA

This class 'fill me in'

David Symons
"""
from odin_data.ipc_tornado_channel import IpcTornadoChannel

# TODO Edit callback to work with INAIRA

class SubSocket(object):
    """
    Subscriber Socket class.
    This class implements an IPC channel subcriber socker and sets up a callback function
    for receiving data from that socket that counts how many images it receives during its lifetime.
    """

    def __init__(self, parent, endpoint):
        """
        Initialise IPC channel as a subscriber, and register the callback.
        :param parent: the class that created this object, a LiveViewer, given so that this object
        can reference the method in the parent
        :param endpoint: the URI address of the socket to subscribe to
        """
        self.parent = parent
        self.endpoint = endpoint
        self.frame_count = 0
        self.channel = IpcTornadoChannel(IpcTornadoChannel.CHANNEL_TYPE_SUB, endpoint=endpoint)
        self.channel.subscribe()
        self.channel.connect()
        # register the get_image method to be called when the ZMQ socket receives a message
        self.channel.register_callback(self.callback)

    def callback(self, msg):
        """
        Handle incoming data on the socket.
        This callback method is called whenever data arrives on the IPC channel socket.
        Increments the counter, then passes the message on to the image renderer of the parent.
        :param msg: the multipart message from the IPC channel
        """
        self.parent.get_frame_updates(msg)

    def cleanup(self):
        """Cleanup channel when the server is closed. Closes the IPC channel socket correctly."""
        self.channel.close()