
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
        model.reset();
    }

    bool InairaMLCppflow::loadModel(std::string file_name)
    {
        /*we use a pointer to the model so we don't have to have it initialized straight away*/
        try{

            model.reset(new cppflow::model(file_name));
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
        if(!model)
        {
            LOG4CXX_ERROR(logger_, "Cannot run model: no model loaded");
            return std::vector<float>(-1);

        }
        
        LOG4CXX_DEBUG(logger_, "Extracting Frame Data");
        const void* frame_data = (void*)frame->get_data_ptr();
        const FrameMetaData meta_data = frame->get_meta_data();
        std::size_t size = frame->get_data_size();
        DataType type = meta_data.get_data_type();
        dimensions_t dims = meta_data.get_dimensions();
        
        LOG4CXX_DEBUG(logger_, "Converting Frame Data");
        std::vector<char> data;
        memcpy(data.data(), frame_data, size);
        cppflow::tensor input = cppflow::cast(data, TF_UINT16, TF_FLOAT);

        LOG4CXX_DEBUG(logger_, "Running model on Frame Data");
        cppflow::model runable_model = *(model.get());
        cppflow::tensor result = runable_model(input);

        LOG4CXX_DEBUG(logger_, "Returning Model Results");
        std::vector<float> return_values;
        // for(int i = 0; i < result, i++)
        // {
        //     return_values.push_back(result[i]);
        // }
        
        return return_values;
    }
}