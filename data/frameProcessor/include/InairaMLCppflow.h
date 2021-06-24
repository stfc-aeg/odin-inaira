
#ifndef INCLUDE_InairaMLCPPFLOW_H_
#define INCLUDE_InairaMLCPPFLOW_H_

// #include <InairaMLFramework.h>
#include "cppflow/cppflow.h"

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <string>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include "Frame.h"

namespace FrameProcessor
{
    class InairaMLCppflow 
    {
        public:
            InairaMLCppflow();
            virtual ~InairaMLCppflow();

            bool loadModel(std::string file_name);
            std::vector<float> runModel(boost::shared_ptr<Frame> frame);

        private:
            boost::scoped_ptr<cppflow::model> model;
            LoggerPtr logger_;
    };
}


#endif /*INCLUDE_InairaMLCPPFLOW_H_*/