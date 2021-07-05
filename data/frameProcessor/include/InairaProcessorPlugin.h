
#ifndef INCLUDE_INAIRAPROCESSORPLUGIN_H_
#define INCLUDE_INAIRAPROCESSORPLUGIN_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"
#include "ClassLoader.h"
#include <boost/algorithm/string.hpp>

#include "version.h"

#include "InairaDefinitions.h"

namespace FrameProcessor
{

    class InairaProcessorPlugin : public FrameProcessorPlugin
    {
        public:
            InairaProcessorPlugin(){};
            virtual ~InairaProcessorPlugin(){};

            int get_version_major() {return ODIN_DATA_VERSION_MAJOR;};
            int get_version_minor() {return ODIN_DATA_VERSION_MINOR;};
            int get_version_patch() {return ODIN_DATA_VERSION_PATCH;};
            std::string get_version_short() {return ODIN_DATA_VERSION_STR_SHORT;};
            std::string get_version_long() {return ODIN_DATA_VERSION_STR;};

        protected:
            virtual void process_frame(boost::shared_ptr<Frame> frame) = 0;

            /** Pointer to logger **/
            LoggerPtr logger_;
    };
}


#endif /*INCLUDE_INAIRAPROCESSORPLUGIN_H_*/