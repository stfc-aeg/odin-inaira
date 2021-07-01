
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

        int64_t buf_dims[] = {dims[0], dims[1], dims[2]};
        int num_dims = 3;

        int dealloc_arg = 123;

        TF_Tensor* buf_tensor = TF_NewTensor(
            TF_UINT8, buf_dims, num_dims, const_cast<void*>(frame_data), size,
            &InairaMLCppflow::test_deallocator, static_cast<void*>(&dealloc_arg)
        );

        LOG4CXX_DEBUG(logger_, "CREATING INPUT TENSOR");
        cppflow::tensor input = cppflow::tensor(buf_tensor);
        LOG4CXX_DEBUG(logger_, "INPUT SHAPE: " << input.shape());
        input = cppflow::cast(input, TF_UINT8, TF_FLOAT);
        input = input / 255.f;
        input = cppflow::expand_dims(input, 0);
        
        // cppflow::tensor input = cppflow::tensor(*(model.get()), "serving_default_input_layer");
        // input.set_data(data)

        LOG4CXX_DEBUG(logger_, "Running model on Frame Data");
        cppflow::model runable_model = *(model.get());
        cppflow::tensor result = runable_model(input);

        LOG4CXX_DEBUG(logger_, result);

        LOG4CXX_DEBUG(logger_, "Max Result: " << cppflow::arg_max(result, 1));
        
        LOG4CXX_DEBUG(logger_, "Returning Model Results");
        std::vector<float> return_values = result.get_data<float>();
        
        return return_values;
    }

    void InairaMLCppflow::test_deallocator(void* buffer, std::size_t len, void* arg)
    {
        int* int_arg = static_cast<int*>(arg);
        std::cout << "Dealloc buffer " << buffer << " len " << len
            << " with arg " << *int_arg << std::endl;
    }
}