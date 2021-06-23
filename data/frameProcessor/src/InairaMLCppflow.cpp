
#include <InairaMLCppflow.h>

namespace FrameProcessor
{
    /*
     * the constructor
     */
    InairaMLCppflow::InairaMLCppflow()
    {
        logger_ = Logger::getLogger("FP.InairaCppFlow");
        logger_->setLevel(Level::getAll());
        LOG4CXX_TRACE(logger_, "Inaira cppflow link loaded");

        // cppflow::model model_("coolpredictor");
    }

    InairaMLCppflow::~InairaMLCppflow()
    {
        LOG4CXX_TRACE(logger_, "Inaira cppflow Link Destructor");
        model_.reset();
    }

    bool InairaMLCppflow::loadModel(std::string file_name)
    {
        /*we use a pointer to the model so we don't have to have it initialized straight away*/
        try{

            model_.reset(new cppflow::model(file_name));
        }
        catch(std::runtime_error& e)
        {
            LOG4CXX_ERROR(logger_, "Error loading model: " << e.what());
            return false;
        }
        return true;
    }

    std::vector<float> InairaMLCppflow::runModel(boost::shared_ptr<Frame> frame)
    {
        if(!model_)
        {
            LOG4CXX_ERROR(logger_, "Cannot run model: no model loaded");
            return std::vector<float>(-1);

        }
        //we gotta somehow convert the shared ptr for data into the correct input type
        void* frame_data_copy = (void*)frame->get_data_ptr();
        // auto input = cppflow::cast(frame.data, TF_UINT, TF_FLOAT);

        // auto result = model_(input); // we'll probs want to change this into something more generic?
        std::vector<float> return_values;
        return_values.push_back(0);
        return return_values;
    }
}