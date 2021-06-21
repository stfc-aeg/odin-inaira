
#include <InairaMLCppflow.h>

namespace FrameProcessor
{
    /*
     * the constructor
     */
    InairaMLCppflow::InairaMLCppflow() :
        model_("./include/cpp_model")
    {
        logger_ = Logger::getLogger("FP.InairaCppFlow");
        logger_->setLevel(Level::getAll());
        LOG4CXX_TRACE(logger_, "Inaira cppflow link loaded");

        // cppflow::model model_("coolpredictor");
    }

    InairaMLCppflow::~InairaMLCppflow()
    {
        LOG4CXX_TRACE(logger_, "Inaira cppflow Link Destructor");
    }

    bool InairaMLCppflow::loadModel(std::string file_name)
    {
        // model_ = cppflow::model(file_name);
        return true;
    }

    std::vector<float> runModel(boost::shared_ptr<Frame> frame)
    {
        //we gotta somehow convert the shared ptr for data into the correct input type
        void* frame_data_copy = (void*)frame->get_data_ptr();
        // auto input = cppflow::cast(frame.data, TF_UINT, TF_FLOAT);

        // auto result = model_(input); // we'll probs want to change this into something more generic?
        std::vector<float> return_values;
        return_values.push_back(0);
        return return_values;
    }
}