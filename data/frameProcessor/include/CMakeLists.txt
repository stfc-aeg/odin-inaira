# Install header files into installation prefix

SET(HEADERS InairaMLCppflow.h
            InairaMLPlugin.h
            InairaProcessorPlugin.h)

INSTALL(FILES ${HEADERS} DESTINATION include/frameProcessor)
