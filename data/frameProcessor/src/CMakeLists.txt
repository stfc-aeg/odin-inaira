
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories(${FRAMEPROCESSOR_DIR}/include ${ODINDATA_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS}
	${CPPFLOW_INCLUDE_DIR} ${TENSORFLOW_INCLUDE_DIR})

# Add Library for each Inaira Plugin
add_library(InairaMLPlugin SHARED InairaMLPlugin.cpp InairaMLCppflow.cpp)

target_include_directories(InairaMLPlugin PRIVATE ../../include ${TENSORFLOW_INCLUDE_DIR})
target_link_libraries (InairaMLPlugin ${TENSORFLOW_LIB})

install(TARGETS InairaMLPlugin DESTINATION lib)
