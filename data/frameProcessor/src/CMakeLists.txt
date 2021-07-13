
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories(${FRAMEPROCESSOR_DIR}/include ${ODINDATA_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS}
	~/libtensorflow2)

# find_library(TENSORFLOW_LIB tensorflow HINT ~/libtensorflow2/lib)
message(STATUS "Tensorflow ${TENSORFLOW_LIB}")
# Add Library for each Inaira Plugin
add_library(InairaMLPlugin SHARED InairaMLPlugin.cpp InairaMLCppflow.cpp)
# add_library(InairaMLCppflow SHARED InairaMLCppflow.cpp)
# add_library(InairaMLFramework SHARED InairaMLFramework.cpp)

target_include_directories(InairaMLPlugin PRIVATE ../../include $ENV{HOME}/libtensorflow2/include)
target_link_libraries (InairaMLPlugin "${TENSORFLOW_LIB}")

install(TARGETS InairaMLPlugin LIBRARY DESTINATION lib)
# install(TARGETS InairaMLCppflow LIBRARY DESTINATION lib)
