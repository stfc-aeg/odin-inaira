
#include <InairaMLCppflow.h>

namespace FrameProcessor
{
    /*
     * the constructor
     */
    InairaMLCppflow::InairaMLCppflow() :
        input_layer_name("serving_default_input_1:0"),
        output_layer_name("StatefulPartitionedCall:0")
    {
        logger_ = Logger::getLogger("FP.InairaCppFlow");
        logger_->setLevel(Level::getAll());
        LOG4CXX_TRACE(logger_, "Inaira cppflow link loaded");

        LOG4CXX_DEBUG(logger_, "SETTING GPU CONFIG OPTIONS");
        
        std::vector<uint8_t> config{0x32,0xb,0x9,0x00,0x00,0x00,0x00,0x00,0x00,0xe0,0x3f,0x20,0x1};

        TFE_ContextOptions* options = TFE_NewContextOptions();
        TFE_ContextOptionsSetConfig(options, config.data(), config.size(), cppflow::context::get_status());
        cppflow::get_global_context() = cppflow::context(options);

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

    bool InairaMLCppflow::setInputLayer(std::string input_name)
    {
        //TODO: potentially use model::get_operations() to check the layer used exists?
        input_layer_name = input_name;
        LOG4CXX_DEBUG(logger_, "Input Layer Name changed to: " << input_name);
        return true;
    }

    bool InairaMLCppflow::setOutputLayer(std::string output_layer)
    {
        //TODO: potentially use model::get_operations() to check the layer used exists?
        output_layer_name = output_layer;
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
        const void* frame_data = frame->get_image_ptr();
        const FrameMetaData meta_data = frame->get_meta_data();
        std::size_t size = frame->get_image_size();
        DataType type = meta_data.get_data_type();
        dimensions_t dims = meta_data.get_dimensions();
        
        int64_t buf_dims[dims.size()];
        for(int i = 0; i < dims.size(); i++)
        {
            buf_dims[i] = dims[i];
        }
        // LOG4CXX_DEBUG(logger_, "DIMS: " << buf_dims);
        int dealloc_arg = 123;

        /*Create a tensor from the frame data. This copies the data into the Tensor format so that
          it can be used by the model and CPPFlow methods
        */
        TF_Tensor* buf_tensor = TF_NewTensor(
            TF_DATA_TYPES[type], buf_dims, dims.size(), const_cast<void*>(frame_data), size,
            &InairaMLCppflow::test_deallocator, static_cast<void*>(&dealloc_arg)
        );

        cppflow::tensor input = cppflow::tensor(buf_tensor);
        input = cppflow::cast(input, TF_DATA_TYPES[type], TF_FLOAT);
        input = cppflow::expand_dims(input, 2);
        input = cppflow::expand_dims(input, 0);

        LOG4CXX_DEBUG(logger_, "Running model on Frame Data");
        cppflow::model runable_model = *(model.get());
        cppflow::tensor result = runable_model({{input_layer_name, input}},
                                               {output_layer_name})[0];
        
        LOG4CXX_DEBUG(logger_, "Returning Model Results");
        std::vector<float> return_values = result.get_data<float>();
        
        return return_values;
    }

    void InairaMLCppflow::test_deallocator(void* buffer, std::size_t len, void* arg)
    {
        int* int_arg = static_cast<int*>(arg);
        // std::cout << "Dealloc buffer " << buffer << " len " << len
        //     << " with arg " << *int_arg << std::endl;
    }
}
