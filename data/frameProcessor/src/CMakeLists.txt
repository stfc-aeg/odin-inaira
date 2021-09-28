
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBOOST_TEST_DYN_LINK)

include_directories(${FRAMEPROCESSOR_DIR}/include ${ODINDATA_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS}
	${CPPFLOW_INCLUDE_DIR} ${TENSORFLOW_INCLUDE_DIR})

# Add Library for each Inaira Plugin
add_library(InairaMLPlugin SHARED InairaMLPlugin.cpp InairaMLCppflow.cpp)
# add_library(InairaMLCppflow SHARED InairaMLCppflow.cpp)
# add_library(InairaMLFramework SHARED InairaMLFramework.cpp)

target_include_directories(InairaMLPlugin PRIVATE ../../include ${TENSORFLOW_INCLUDE_DIR})
target_link_libraries (InairaMLPlugin "${TENSORFLOW_LIBRARIES}")

install(TARGETS InairaMLPlugin LIBRARY DESTINATION lib)
# install(TARGETS InairaMLCppflow LIBRARY DESTINATION lib)

add_library(PcoCameraProcessPlugin SHARED PcoCameraProcessPlugin.cpp)
target_include_directories(PcoCameraProcessPlugin PRIVATE ../../include)

install(TARGETS PcoCameraProcessPlugin LIBRARY DESTINATION lib)

