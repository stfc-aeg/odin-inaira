
#ifndef INCLUDE_InairaMLCPPFLOW_H_
#define INCLUDE_InairaMLCPPFLOW_H_

// #include <InairaMLFramework.h>
#include "cppflow/cppflow.h"
#include <tensorflow/c/tf_tensor.h>

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
    /*List of Tensorflow Datatypes, mapped to the Odin Data Datatype enum*/
    const TF_DataType TF_DATA_TYPES[] = {TF_UINT8, TF_UINT8, TF_UINT16, TF_UINT32, TF_UINT64, TF_FLOAT};

    class InairaMLCppflow 
    {
        public:
            InairaMLCppflow();
            virtual ~InairaMLCppflow();

            bool loadModel(std::string file_name);
            bool setInputLayer(std::string input_layer);
            bool setOutputLayer(std::string output_layer);
            std::vector<float> runModel(boost::shared_ptr<Frame> frame);

            std::string input_layer_name;
            std::string output_layer_name;

        private:
            static void test_deallocator(void* buffer, std::size_t len, void* arg);
            boost::scoped_ptr<cppflow::model> model;
            LoggerPtr logger_;
    };
}


#endif /*INCLUDE_InairaMLCPPFLOW_H_*/