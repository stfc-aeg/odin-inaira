#
# FindTensorflow.cmake
#
#
# The MIT License
#
# Copyright (c) 2016 MIT and Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# Finds the TENSORFLOW library. This module defines:
#   - TENSORFLOW_INCLUDE_DIR, directory containing headers
#   - TENSORFLOW_FOUND, whether TENSORFLOW has been found
# Define TENSORFLOW_ROOT_DIR if TENSORFLOW is installed in a non-standard location.

message ("\nLooking for Tensorflow")

if (TENSORFLOW_ROOT_DIR)
    message (STATUS "Searching Tensorflow Root Dir: ${TENSORFLOW_ROOT_DIR}")
endif()

# Find header files
if(TENSORFLOW_ROOT_DIR)
    find_path(
            TENSORFLOW_INCLUDE_DIR tensorflow/c/c_api.h
            PATHS ${TENSORFLOW_ROOT_DIR}/include
            NO_DEFAULT_PATH
    )
else()
    find_path(TENSORFLOW_INCLUDE_DIR tensorflow/c/c_api.h)
endif()

#find library
if(TENSORFLOW_ROOT_DIR)
    find_library(
        TENSORFLOW_LIB NAMES tensorflow
        PATHS ${TENSORFLOW_ROOT_DIR}/lib
        NO_DEFAULT_PATH
    )
    find_library(TENSORFLOW_FRAMEWORK_LIBRARY
        NAMES tensorflow_framework
        PATHS ${TENSORFLOW_ROOT_DIR}/lib)
else()
    find_library(TENSORFLOW_LIB NAMES tensorflow)
    find_library(TENSORFLOW_FRAMEWORK_LIBRARY NAMES tensorflow_framework)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(TENSORFLOW
    DEFAULT_MSG
    TENSORFLOW_INCLUDE_DIR
    TENSORFLOW_LIB
    TENSORFLOW_FRAMEWORK_LIBRARY
)

if(TENSORFLOW_INCLUDE_DIR AND TENSORFLOW_LIB)
    message(STATUS "Found Tensorflow: ${TENSORFLOW_LIB}")
    set(TENSORFLOW_FOUND TRUE)
else()
    set(TENSORFLOW_FOUND FALSE)
endif()

if(NOT TENSORFLOW_FOUND)
    message(STATUS "Could not find the Tensorflow Library.")
    message(STATUS "Libraries Found: ${TENSORFLOW_LIB}")
    message(STATUS "Include Dir Found: ${TENSORFLOW_INCLUDE_DIR}")
endif()
