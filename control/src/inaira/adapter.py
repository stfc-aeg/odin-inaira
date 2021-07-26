"""
Adapter for ODIN INAIRA

This class 'fill me in'

David Symons, Some random apprencitce Tim found
"""
import logging
import tornado
import time
import sys
from concurrent import futures

from tornado.ioloop import IOLoop, PeriodicCallback
from tornado.concurrent import run_on_executor
from tornado.escape import json_decode

from odin.adapters.adapter import ApiAdapter, ApiAdapterResponse, request_types, response_types
from odin.adapters.parameter_tree import ParameterTree, ParameterTreeError
from odin._version import get_versions

from .odin_inaira import OdinInaira, OdinInairaError

class OdinInairaAdapter(ApiAdapter):

    
    def __init__(self, **kwargs):
        """Initialize the WorkshopAdapter object.

        This constructor initializes the WorkshopAdapter object.

        :param kwargs: keyword arguments specifying options
        """
        # Intialise superclass
        super(OdinInairaAdapter, self).__init__(**kwargs)

        

        self.odin_inaira = OdinInaira()

        logging.debug('OdinInairaAdapter loaded')

    @response_types('application/json', default='application/json')
    def get(self, path, request):
        """Handle an HTTP GET request.

        This method handles an HTTP GET request, returning a JSON response.

        :param path: URI path of request
        :param request: HTTP request object
        :return: an ApiAdapterResponse object containing the appropriate response
        """
        try:
            response = self.odin_inaira.get(path)
            status_code = 200
        except ParameterTreeError as e:
            response = {'error': str(e)}
            status_code = 400

        content_type = 'application/json'

        return ApiAdapterResponse(response, content_type=content_type,
                                  status_code=status_code)

    @request_types('application/json')
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
            self.odin_inaira.set(path, data)
            response = self.odin_inaira.get(path)
            status_code = 200
        except OdinInairaError as e:
            response = {'error': str(e)}
            status_code = 400
        except (TypeError, ValueError) as e:
            response = {'error': 'Failed to decode PUT request body: {}'.format(str(e))}
            status_code = 400

        logging.debug(response)

        return ApiAdapterResponse(response, content_type=content_type,
                                  status_code=status_code)

    def delete(self, path, request):
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

    # def cleanup(self):
    #     """Clean up adapter state at shutdown.

    #     This method cleans up the adapter state when called by the server at e.g. shutdown.
    #     It simplied calls the cleanup function of the odinInaira instance.
    #     """
    #     self.OdinInaira.cleanup()