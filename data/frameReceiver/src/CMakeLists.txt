
set(CMAKE_INCLUDE_CURRENT_DIR on)
ADD_DEFINITIONS(-DBUILD_DIR="${CMAKE_BINARY_DIR}")

message(STATUS "BOOST:: ${Boost_INCLUDE_DIRS}")

include_directories(${FRAMERECEIVER_DIR}/include ${ODINDATA_INCLUDE_DIRS}
	${Boost_INCLUDE_DIRS} ${LOG4CXX_INCLUDE_DIRS}/.. ${ZEROMQ_INCLUDE_DIRS})

# Add library for Inaira frame decoder
add_library(InairaFrameDecoder SHARED InairaFrameDecoder.cpp InairaFrameDecoderLib.cpp)

install(TARGETS InairaFrameDecoder LIBRARY DESTINATION lib)

if (PCOCAMERA_FOUND)


	link_directories(${PCOCAMERA_LIBRARY_DIR})

	# Add library for PCO CameraLink Decoder
	add_library(PcoCameraLinkFrameDecoder SHARED
		PcoCameraLinkFrameDecoder.cpp
		PcoCameraLinkFrameDecoderLib.cpp
		PcoCameraLinkController.cpp
		PcoCameraStateMachine.cpp
		ParamContainer.cpp)

	target_include_directories(PcoCameraLinkFrameDecoder PRIVATE ${PCOCAMERA_INCLUDES})

	target_link_libraries(PcoCameraLinkFrameDecoder PRIVATE ${PCOCAMERA_LIBRARIES})

	install(TARGETS PcoCameraLinkFrameDecoder LIBRARY DESTINATION lib)

endif()