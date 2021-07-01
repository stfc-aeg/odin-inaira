/*

methods we'll need:
load model
send data to model
get result from model (store in... */


#include <InairaMLPlugin.h>
#include "version.h"
#include "cppflow/cppflow.h"  /*TODO: Remove when ready*/

namespace FrameProcessor
{
    const std::string InairaMLPlugin::CONFIG_MODEL_PATH = "model_path";

    /**
     * The constructor
     */
    InairaMLPlugin::InairaMLPlugin()
    {
        //Setup logging
        logger_ = Logger::getLogger("FP.InairaMLPlugin");
        logger_->setLevel(Level::getAll());
        LOG4CXX_TRACE(logger_, "InairaMLPlugin version " <<
                      this->get_version_long() << " loaded.");
    }

    InairaMLPlugin::~InairaMLPlugin()
    {
        LOG4CXX_TRACE(logger_, "InairaMLPlugin Destructor.");
    }


    /**
     * Configure the Inaira plugin.  This receives an IpcMessage which should be processed
     * to configure the plugin, and any response can be added to the reply IpcMessage.  This
     * plugin supports the following configuration parameters:
     * 
     * - xx_      <=> xx
     *
     * \param[in] config - Reference to the configuration IpcMessage object.
     * \param[in] reply - Reference to the reply IpcMessage object.
     */
    void InairaMLPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
    {
        //send configuration to the plugin
        if(config.has_param(InairaMLPlugin::CONFIG_MODEL_PATH))
        {
            model_path = config.get_param<std::string>(
                InairaMLPlugin::CONFIG_MODEL_PATH
            );
            model_.loadModel(model_path);

            //hyjack the config method real quick to trigger the model running?

            LOG4CXX_DEBUG(logger_, "Testing new model with Dummy Frame");
            FrameMetaData frame_meta;
            cppflow::tensor image_input = cppflow::decode_jpeg(cppflow::read_file(std::string("dog.jpg")));
            LOG4CXX_DEBUG(logger_, "Image Tensor Shape: " << image_input.shape());
            std::vector<uint8_t> image_data = image_input.get_data<uint8_t>();
            LOG4CXX_DEBUG(logger_, "Image Data Size: " << image_data.size());

            frame_meta.set_dataset_name("dog");
            frame_meta.set_data_type(raw_8bit);
            frame_meta.set_frame_number(0);
            dimensions_t dims(3);
            dims[0] = 600;
            dims[1] = 800;
            dims[2] = 3;
            frame_meta.set_dimensions(dims);
            frame_meta.set_compression_type(no_compression);

            const std::size_t frame_size = image_data.size() * sizeof(uint8_t);

            boost::shared_ptr<Frame> test_frame;
            test_frame = boost::shared_ptr<Frame>(new DataBlockFrame(frame_meta, frame_size));

            // std::memset(test_frame->get_data_ptr(), 25, frame_size);
            std::memcpy(test_frame->get_data_ptr(), image_data.data(), frame_size);
            

            std::vector<float> result = model_.runModel(test_frame);
            std::ostringstream result_string;
            std::copy(result.begin(),
                      result.end(),
                      std::ostream_iterator<float>(result_string, ","));
            LOG4CXX_DEBUG(logger_, "Result: " << result_string.str());
        }
    }

    void InairaMLPlugin::requestConfiguration(OdinData::IpcMessage& reply)
    {
        //return the config of the plugin

        std::string base_str = get_name() + "/";
        reply.set_param(base_str + InairaMLPlugin::CONFIG_MODEL_PATH, model_path);
    }

    void InairaMLPlugin::status(OdinData::IpcMessage& status)
    {
        //return the status of the plugin
        LOG4CXX_DEBUG(logger_, "Status requested for InairaMLPlugin");

    }

    bool InairaMLPlugin::reset_statistics(void)
    {
        return true;
    }

    void InairaMLPlugin::process_frame(boost::shared_ptr<Frame> frame)
    {
        std::vector<float> data = model_.runModel(frame);
        // do a something with the results from running the ML
        this->push(frame);
    }
}