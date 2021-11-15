
import logging
import tornado
import time
import sys
from concurrent import futures

from tornado.ioloop import IOLoop, PeriodicCallback
from tornado.concurrent import run_on_executor
from tornado.escape import json_decode

from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, request_types, response_types
from odin.adapters.parameter_tree import ParameterTreeError
from odin._version import get_versions

from .inaira_control import InairaControl, InairaControlError

DEFAULT_ENDPOINT = 'tcp://127.0.0.1:530'
DEFAULT_LIVE_IMAGE = False
ENDPOINTS = 'inaira_endpoints'
LIVE_IMAGE = "process_live_image"

class InairaControlAdapter(ApiAdapter):
     
    def __init__(self, **kwargs):
        logging.debug("Odin Control Adapter has started") 
        super(InairaControlAdapter, self).__init__(**kwargs)

        # Set endpoints for zmq connections
        if self.options.get(ENDPOINTS, False):
            endpoints = [x.strip() for x in self.options.get(ENDPOINTS, "").split(',')]
        else:
            logging.debug("Setting default endpoint of '%s'", self.options.get(DEFAULT_ENDPOINT, ""))
            endpoints = self.options.get(DEFAULT_ENDPOINT, "")
        live_image = self.options.get(LIVE_IMAGE, DEFAULT_LIVE_IMAGE)
        logging.debug("INAIRA provide live image: %s", live_image)

        # Status Refresh and Ctrl endpoint
        ctrl_endpoint = self.options.get("ctrl_endpoint", "")
        status_refresh = int(self.options.get("status_loop_time", 1000))

        self.inaira_control = InairaControl(endpoints, live_image, ctrl_endpoint, status_refresh)
  
    # get
    @request_types('application/json')
    @response_types('application/json', default='application/json')
    def get(self, path, request):  
        """Handle an HTTP GET request.

        This method handles an HTTP GET request, returning a JSON response.

        :param path: URI path of request
        :param request: HTTP request object
        :return: an ApiAdapterResponse object containing the appropriate response
        """
        try:
            response = self.inaira_control.get(path)
            status_code = 200
        except ParameterTreeError as e:
            response = {'error': str(e)}
            status_code = 400

        content_type = 'application/json'
        return ApiAdapterResponse(response, content_type=content_type,
                                  status_code=status_code)

    # put
    @response_types('application/json', default='application/json')
    def put(self, path, request):
        """Handle an HTTP PUT request.

        This method handles an HTTP PUT request, returning a JSON response.

        :param path: URI path of request
        :param request: HTTP request object
        :return: an ApiAdapterResponse object containing the appropriate response
        """
        content_type = 'application/json'

        try:
            data = json_decode(request.body)
            self.inaira_control.set(path, data)
            response = self.inaira_control.get(path)
            status_code = 200
        except InairaControlError as e:
            response = {'error': str(e)}
            status_code = 400
        except (TypeError, ValueError) as e:
            response = {'error': 'Failed to decode PUT request body: {}'.format(str(e))}
            status_code = 400

        logging.debug(response)

        return ApiAdapterResponse(response, content_type=content_type,
                                  status_code=status_code)

    # delete
    def delete(elf, path, requet):
        """Handle an HTTP DELETE request.

        This method handles an HTTP DELETE request, returning a JSON response.

        :param path: URI path of request
        :param request: HTTP request object
        :return: an ApiAdapterResponse object containing the appropriate response
        """
        response = 'OdinInairaAdapter: DELETE on path {}'.format(path)
        status_code = 200

        logging.debug(response)

        return ApiAdapterResponse(response, status_code=status_code)

    # cleanup
    def cleanup(self):
        """Clean up adapter state at shutdown.

        This method cleans up the adapter state when called by the server at e.g. shutdown.
        It simplied calls the cleanup function of the odinInaira instance.
        """
        self.inaira_control.cleanup()

    # initialize