import logging
import queue
import sys
import threading
import time

import click
import numpy as np
import struct

from skimage import io
from os import listdir, walk
from os.path import join

from odin_data.shared_buffer_manager import SharedBufferManager, SharedBufferManagerException
from odin_data.ipc_channel import IpcChannel, IpcChannelException
from odin_data.ipc_message import IpcMessage, IpcMessageException

from .frame_producer_config import FrameProducerConfig

class FrameProducer():

    def __init__(self, config_file):

        # Create a configuration object with default settings, loading from a config file if
        # specified
        self.config = FrameProducerConfig(config_file=config_file)

        # Create a logger and set up a console handler with the appropriate format
        self.logger = logging.getLogger('frame_producer')
        ch = logging. StreamHandler(sys.stdout)
        formatter = logging.Formatter(fmt=self.config.log_format, datefmt="%d/%m/%y %H:%M:%S")
        ch.setFormatter(formatter)
        self.logger.addHandler(ch)

        # Set the logging level if defined legally in the configuration
        log_level = getattr(logging, self.config.log_level.upper(), None)
        if log_level:
            ch.setLevel(log_level)
            self.logger.setLevel(log_level)

        self.logger.info("FrameProducer starting up")

        # Initialise the shared buffer manager
        self.shared_buffer_manager = SharedBufferManager(
            self.config.shared_buffer,
            self.config.shared_mem_size,
            self.config.shared_buffer_size,
            remove_when_deleted=True,
            boost_mmap_mode=self.config.boost_mmap_mode
        )
        self.logger.debug(
            "Mapped shared buffer manager %s id %d with %d buffers of size %d",
            self.config.shared_buffer,
            self.shared_buffer_manager.get_manager_id(),
            self.shared_buffer_manager.get_num_buffers(),
            self.shared_buffer_manager.get_buffer_size()
        )

        # Create the frame ready channel and bind it to the endpoint
        self.ready_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_PUB)
        self.ready_channel.bind(self.config.ready_endpoint)
        self.logger.debug(
            "Created frame ready IPC channel on endpoint %s",
            self.config.ready_endpoint
        )

        # Create the frame release channel, bind it to the endpoint and set the default
        # subscription on the channel
        self.release_channel = IpcChannel(IpcChannel.CHANNEL_TYPE_SUB)
        self.release_channel.bind(self.config.release_endpoint)
        self.release_channel.subscribe(b'')
        self.logger.debug(
            "Created frame release IPC channel on endpoint %s",
            self.config.release_endpoint
        )

        # Create the queue to contain available buffer IDs and precharge it with all available
        # shared memory buffers
        self.free_buffer_queue = queue.Queue()
        for idx in range(self.shared_buffer_manager.get_num_buffers()):
            self.free_buffer_queue.put(idx)
        self.logger.debug(
            "Created free buffer queue with %d buffers pre-charged",
            self.free_buffer_queue.qsize()
        )

        # Initialise msg index counter
        self._next_msg_id = 0

        # Set the internal run flag to true
        self._run = True

        # Create a thread to handle frame release messages
        self.release_thread = threading.Thread(target=self.process_release)

        # Start the release channel processing thread
        self.release_thread.start()

    def get_next_msg_id(self):
        """Increment and return the next message ID for IPC messages."""
        self._next_msg_id += 1
        return self._next_msg_id

    # Enumerate the data type so it can be sent in frame header
    def get_dtype_enumeration(random, img_dtype):
        list_of_dtypes = ["unknown","uint8","uint16","uint32","uint64","float"]
        # Is theee data type contained within the odin data eenuumeeration list?
        if img_dtype in list_of_dtypes:
            enumerated_dtype = list_of_dtypes.index(img_dtype)

        else:
            enumerated_dtype = 0
        
        # Return the enumerated data type
        return enumerated_dtype

    # Convert image from rgb to gray
    def rgb2gray(random, img):
        #Convert from rgb to gray
        return np.dot(img[...,:3],[0.2989, 0.5870, 0.1140])

    def run(self, frames, fps, imgs_path):

        try:
        #region
            """Run the frame producer main event loop."""

            # Check if config from camera emulator is not None
            if frames == None:
                frames = self.config.frames

            if fps == None:
                fps = self.config.frames_per_second

            if imgs_path == None:
                imgs_path = self.config.testfile_path


            # Allow the IPC channels to connect and then notify the frame processor of the buffer
            # configuration
            time.sleep(1.0)
            self.notify_buffer_config()

            #Load the test images 

            totalimages = 0

            # Number of files in directory
            for files in walk(imgs_path):
                for Files in files:
                    totalimages += 1

            # Loop over the specified number of frames and transmit them
            self.logger.info("Sending %d frames to processor\n", frames)

            self.frame = 0
            self.send_frames = True

            while self.send_frames:

                try:
                    # Frame producer code here 
                    self.logger.debug(" ----- Beginning creation of frame %d -----\n\n", self.frame)

                    # Set image path based on frame number
                    testimage = listdir(imgs_path)[self.frame%totalimages]

                    # Load image
                    vals = io.imread(join(imgs_path,testimage))

                    # Is the image RGB or Grayscale?
                    if (len(vals.shape) >= 3):
                        # Convert to Grayscale
                        vals = self.rgb2gray(vals)
                        
                        # Correct data type
                        vals = vals.astype(np.uint8)

                    # Debugging of image loading
                    self.logger.debug("The filename is " + testimage)
                    self.logger.debug("The first 10 frame values: " + str(vals[0][:10]))

                    # Pop the first free buffer off the queue
                    # TODO deal with empty queue??
                    buffer = self.free_buffer_queue.get()

                    # Split image shape from (x, y) into x and y
                    imageshape = vals.shape
                    imagewidth = imageshape[0]
                    imageheight = imageshape[1]
                    self.logger.debug("Width " + str(imagewidth) + " Height " + str(imageheight) + "\n")

                    # What is the dtype outputting
                    self.logger.debug("Data Type: " + str(vals.dtype))
                    self.logger.debug("Data Type Enumeration: " + str(self.get_dtype_enumeration(vals.dtype.name)) + "\n")

                    # Create struct with these parameters for the header
                    # frame_header, (currently ignoring)frame_state, frame_start_time_secs, frame_start_time_nsecs, frame_width, frame_height, frame_data_type, frame_size
                    header = struct.pack("iiiii", self.frame, imagewidth, imageheight, self.get_dtype_enumeration(vals.dtype.name), vals.size)

                    # Copy the image nparray directly into the buffer as bytes
                    self.logger.debug("Filling frame %d into buffer %d", self.frame, buffer)
                    self.shared_buffer_manager.write_buffer(buffer, header + vals.tobytes())

                    # Notify the processor that the frame is ready in the buffer
                    self.notify_frame_ready(self.frame, buffer)

                    self.logger.debug("----- Creation of frame %d complete -----\n\n", self.frame)

                    # Sent frames at a set frame rate using config
                    time.sleep(1/fps)

                    # Next frame
                    self.frame += 1

                    # Are all frames sent?
                    if (self.frame == (frames)):
                        self.send_frames = False

                except KeyboardInterrupt:
                    self.send_frames = False

            
            #endregion
        except Exception:
            self.send_frames = False
            self.logger.warning("Error in frame producer")

        self.logger.info("Frame producer stopping")
       
        

    def stop(self):
        # Clear the run flag and wait for the release processing thread to terminate
        self._run = False
        self.release_thread.join()

        self.logger.info("Frame Producer Terminating")

    def process_release(self):
        """Handle buffer release notifications from the frame processor."""

        self.logger.debug("Release processing thread started")

        # Keep looping while the global run flag is set
        while self._run:

            # Poll the release channel for frame release messages with a short timeout, allowing 
            # this loop to 'tick'
            if self.release_channel.poll(100):

                release_msg = IpcMessage(from_str=self.release_channel.recv())

                # Extract the message type and value fields
                msg_type = release_msg.get_msg_type()
                msg_val = release_msg.get_msg_val()

                # Handle a frame release notification
                if msg_type == 'notify' and msg_val == 'frame_release':

                    # Get the buffer ID and frame number from the message parameters
                    buffer_id = release_msg.get_param('buffer_id')
                    frame = release_msg.get_param("frame")

                    # Push the released buffer onto the free buffer queue
                    self.free_buffer_queue.put(buffer_id)

                    self.logger.debug(
                        "Got frame release notification for frame %d in buffer %d \n",
                        frame, buffer_id
                    )

                # Handle an async buffer config request from the processor
                elif msg_type == 'cmd' and msg_val == 'request_buffer_config':
                    self.logger.debug("Got buffer config request from processor")
                    self.notify_buffer_config()

                # If an unexpected message is received on the release channel, just log a message
                else:
                    self.logger.warn(
                        "Got Unexpected IPC message on frame release channel: %s ",
                        release_msg
                    )

        self.logger.debug("Release processing thread terminating")

    def notify_buffer_config(self):
        """Notify the processor of the current shared buffer configuration."""
        self.logger.info("Notifying downstream processor of shared buffer configuration")
        config_msg = IpcMessage(msg_type='notify', msg_val='buffer_config', id=self.get_next_msg_id())
        config_msg.set_param('shared_buffer_name', self.config.shared_buffer)
        self.ready_channel.send(config_msg.encode())

    def notify_frame_ready(self, frame, buffer):
        """Notify the processor that a frame is ready in a shared buffer."""
        ready_msg = IpcMessage(msg_type='notify', msg_val='frame_ready', id=self.get_next_msg_id())
        ready_msg.set_param('frame', frame)
        ready_msg.set_param('buffer_id', buffer)
        self.ready_channel.send(ready_msg.encode())

@click.command()
@click.option('--config', help="The path to the required yml config file.")
def main(config):
    fp = FrameProducer(config)

if __name__ == "__main__":
    main()