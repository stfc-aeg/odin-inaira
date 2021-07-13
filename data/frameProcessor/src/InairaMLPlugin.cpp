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
    const std::string InairaMLPlugin::CONFIG_MODEL_INPUT_LAYER = "model_input_layer";
    const std::string InairaMLPlugin::CONFIG_MODEL_OUTPUT_LAYER = "model_output_layer";
    const std::string InairaMLPlugin::CONFIG_DECODE_IMG_HEADER = "decode_header";
    const std::string InairaMLPlugin::CONFIG_TEST_MODEL = "test_model";
    const std::string InairaMLPlugin::CONFIG_MODEL_TEST_IMG_PATH = "test_img_path";
    const std::string InairaMLPlugin::CONFIG_RESULT_DEST = "result_socket_addr";

    /**
     * The constructor
     */
    InairaMLPlugin::InairaMLPlugin() :
        publish_socket_(ZMQ_PUB),
        decode_header(false),
        avg_process_time(0),
        total_process_time(0),
        num_processed(0)
    {
        //Setup logging
        logger_ = Logger::getLogger("FP.InairaMLPlugin");
        logger_->setLevel(Level::getAll());
        LOG4CXX_TRACE(logger_, "InairaMLPlugin version " <<
                      this->get_version_long() << " loaded.");

        classes[0] = "Bad";
        classes[1] = "Good";
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

        if(config.has_param(InairaMLPlugin::CONFIG_MODEL_INPUT_LAYER))
        {
            std::string input_name = config.get_param<std::string>(InairaMLPlugin::CONFIG_MODEL_INPUT_LAYER);
            model_.setInputLayer(input_name);
        }
        if(config.has_param(InairaMLPlugin::CONFIG_MODEL_OUTPUT_LAYER))
        {
            std::string output_name = config.get_param<std::string>(InairaMLPlugin::CONFIG_MODEL_OUTPUT_LAYER);
            model_.setOutputLayer(output_name);
        }
        if(config.has_param(InairaMLPlugin::CONFIG_DECODE_IMG_HEADER))
        {
            bool config_decode_header = config.get_param<bool>(InairaMLPlugin::CONFIG_DECODE_IMG_HEADER);
            decode_header = config_decode_header;
        }
        //send configuration to the plugin
        if(config.has_param(InairaMLPlugin::CONFIG_MODEL_PATH))
        {
            model_path = config.get_param<std::string>(
                InairaMLPlugin::CONFIG_MODEL_PATH
            );
            model_.loadModel(model_path);
        }
        if(config.has_param(InairaMLPlugin::CONFIG_TEST_MODEL) && config.get_param<bool>(InairaMLPlugin::CONFIG_TEST_MODEL))
        {
            //hyjack the config method real quick to trigger the model running?

            LOG4CXX_DEBUG(logger_, "Testing model with Dummy Frame");
            // FrameMetaData frame_meta;
            std::string img_path = config.get_param<std::string>(InairaMLPlugin::CONFIG_MODEL_TEST_IMG_PATH);
            // std::string bad_img_path =  "/aeg_sw/work/projects/inaira/casting/cppflow/casting_model/cast_def_0_112.jpeg";

            Inaira::FrameHeader header;
            header.frame_number = 0;
            header.frame_data_type = raw_8bit;
            header.frame_width = 300;
            header.frame_height = 300;
            header.frame_size = 300*300;

            LOG4CXX_DEBUG(logger_, "Loading Image");
            cppflow::tensor image_input = cppflow::decode_jpeg(cppflow::read_file(img_path), 1);
            std::vector<uint8_t> image_data = image_input.get_data<uint8_t>();

            const std::size_t frame_size = image_data.size();

            boost::shared_ptr<Frame> test_frame;
            FrameMetaData empty_meta;
            test_frame = boost::shared_ptr<Frame>(new DataBlockFrame(empty_meta, frame_size  + sizeof(header)));

            void* data_ptr = static_cast<void*>(
                static_cast<char*>(test_frame->get_data_ptr()) + sizeof(Inaira::FrameHeader)
            );
            memcpy(test_frame->get_data_ptr(), &header, sizeof(header));
            memcpy(data_ptr, image_data.data(), frame_size);
            
            process_frame(test_frame);
        }

    }

    void InairaMLPlugin::requestConfiguration(OdinData::IpcMessage& reply)
    {
        //return the config of the plugin

        std::string base_str = get_name() + "/";
        reply.set_param(base_str + InairaMLPlugin::CONFIG_MODEL_PATH, model_path);
        reply.set_param(base_str + InairaMLPlugin::CONFIG_MODEL_INPUT_LAYER, model_.input_layer_name);
        reply.set_param(base_str + InairaMLPlugin::CONFIG_MODEL_OUTPUT_LAYER, model_.output_layer_name);
    }

    void InairaMLPlugin::status(OdinData::IpcMessage& status)
    {
        //return the status of the plugin
        LOG4CXX_DEBUG(logger_, "Status requested for InairaMLPlugin");

        std::string base_str = get_name() + "/";
        status.set_param(base_str + "avg_process_time", avg_process_time);
        status.set_param(base_str + "num_processed", num_processed);


    }

    bool InairaMLPlugin::reset_statistics(void)
    {
        return true;
    }

    void InairaMLPlugin::process_frame(boost::shared_ptr<Frame> frame)
    {
        LOG4CXX_DEBUG(logger_, "Process Frame Called");
        boost::posix_time::ptime then = boost::posix_time::microsec_clock::local_time();
        if(decode_header)
        {
            decodeHeader(frame);
        }

        std::vector<float> result = model_.runModel(frame);
        boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
        uint32_t frame_process_time = (now - then).total_milliseconds();
        
        
        total_process_time += frame_process_time;
        num_processed += 1;
        avg_process_time = total_process_time / num_processed;

        LOG4CXX_DEBUG(logger_, "Frame Processing took " << frame_process_time <<"ms");
        LOG4CXX_DEBUG(logger_, "Average Processing time over " << num_processed << "Frames: " << avg_process_time);

        int max = int(std::distance(result.begin(), max_element(result.begin(), result.end())));
        LOG4CXX_DEBUG(logger_, "Image Result: " << classes[max] << ", score: " << result[max]);
        if(max == 0)
            frame->meta_data().set_dataset_name("defective");
        else
            frame->meta_data().set_dataset_name("good");

        sendResults(frame->get_frame_number(), frame_process_time, result);
        this->push(frame);
    }

    void InairaMLPlugin::decodeHeader(boost::shared_ptr<Frame> frame)
    {
        LOG4CXX_DEBUG(logger_, "Decoding Frame Header");
        
        Inaira::FrameHeader* hdr_ptr = static_cast<Inaira::FrameHeader*>(frame->get_data_ptr());

            FrameMetaData metadata;

            metadata.set_dataset_name("inaira");
            metadata.set_data_type((DataType)hdr_ptr->frame_data_type);
            metadata.set_frame_number(hdr_ptr->frame_number);
            metadata.set_compression_type(no_compression);
            dimensions_t dims(2);
            dims[0] = hdr_ptr->frame_height;
            dims[1] = hdr_ptr->frame_width;
            metadata.set_dimensions(dims);

            frame->set_meta_data(metadata);
            frame->set_image_offset(sizeof(Inaira::FrameHeader));
            frame->set_image_size(hdr_ptr->frame_height*hdr_ptr->frame_width * sizeof(uint8_t));
    }

    void InairaMLPlugin::sendResults(uint32_t frame_number, uint32_t process_time, std::vector<float> results)
    {
        //something describing frame? probs just frame number
        LOG4CXX_DEBUG(logger_, "Creating Json structure");
        rapidjson::Document doc;
        doc.SetObject();

        rapidjson::Value key_num("frame_number", doc.GetAllocator());
        rapidjson::Value frame_num(frame_number);
        doc.AddMember(key_num, frame_num, doc.GetAllocator());

        rapidjson::Value key_time("process_time", doc.GetAllocator());
        rapidjson::Value frame_time(process_time);
        doc.AddMember(key_time, frame_time, doc.GetAllocator());
        //do a loop for the values in the result vector
        
        LOG4CXX_DEBUG(logger_, "Adding Array to json");
        
        rapidjson::Value keyResults("result", doc.GetAllocator());
        rapidjson::Value valResults(rapidjson::kArrayType);
        for(int i = 0; i < results.size(); i++)
        {
            rapidjson::Value value(results[i]);
            valResults.PushBack(value, doc.GetAllocator());
        }
        doc.AddMember(keyResults, valResults, doc.GetAllocator());

        rapidjson::StringBuffer buffer;
        buffer.Clear();
        rapidjson::Writer<rapidjson::StringBuffer, rapidjson::UTF8<> > writer(buffer);
        doc.Accept(writer);
        //print the json out real quick for debug purposes
        LOG4CXX_DEBUG(logger_, "Json:" << buffer.GetString());

        publish_socket_.send(buffer.GetString());

    }
}