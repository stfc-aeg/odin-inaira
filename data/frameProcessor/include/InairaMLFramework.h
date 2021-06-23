
#ifndef INCLUDE_InairaMLFRAMEWORK_H_
#define INCLUDE_InairaMLFRAMEWORK_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <string>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include <boost/shared_ptr.hpp>
#include "Frame.h"

namespace FrameProcessor
{
    class InairaMLFramework
    {
        public:
            InairaMLFramework();
            virtual ~InairaMLFramework();

            virtual bool loadModel(std::string file_name);
            virtual std::vector<float> runModel(boost::shared_ptr<Frame> frame) = 0;
        
        protected:
            LoggerPtr logger_;
    };
}

#endif /*INCLUDE_InairaMLFRAMEWORK_H_*/