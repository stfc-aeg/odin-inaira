/*

methods we'll need:
load model
send data to model
get result from model (store in... */


#include <InairaMLPlugin.h>
#include "version.h"
#include "Json.h"

namespace FrameProcessor
{
    const std::string InairaMLPlugin::CONFIG_MODEL_PATH = "model_path";
    const std::string InairaMLPlugin::CONFIG_MODEL_INPUT_LAYER = "model_input_layer";
    const std::string InairaMLPlugin::CONFIG_MODEL_OUTPUT_LAYER = "model_output_layer";
    const std::string InairaMLPlugin::CONFIG_DECODE_IMG_HEADER = "decode_header";
    const std::string InairaMLPlugin::CONFIG_RESULT_DEST = "result_socket_addr";
    const std::string InairaMLPlugin::CONFIG_SEND_RESULTS = "send_results";
    const std::string InairaMLPlugin::CONFIG_SEND_IMAGE = "send_image";


    /**
     * The constructor
     */
    InairaMLPlugin::InairaMLPlugin() :
        publish_socket_(ZMQ_PUB),
        is_bound_(false),
        decode_header(false),
        send_results_(false),
        send_image_(false),
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
        if(config.has_param(InairaMLPlugin::CONFIG_SEND_RESULTS))
        {
            send_results_ = config.get_param<bool>(InairaMLPlugin::CONFIG_SEND_RESULTS);
        }
        if(config.has_param(InairaMLPlugin::CONFIG_SEND_IMAGE))
        {
            send_image_ = config.get_param<bool>(InairaMLPlugin::CONFIG_SEND_IMAGE);
        }
        if(config.has_param(InairaMLPlugin::CONFIG_RESULT_DEST))
        {
            setSocketAddr(config.get_param<std::string>(InairaMLPlugin::CONFIG_RESULT_DEST));
        }
        //send configuration to the plugin
        if(config.has_param(InairaMLPlugin::CONFIG_MODEL_PATH))
        {
            model_path = config.get_param<std::string>(
                InairaMLPlugin::CONFIG_MODEL_PATH
            );
            model_.loadModel(model_path);
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
        total_process_time = 0;
        num_processed = 0;
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

        if(send_results_)
        {
            std::string results = sendResults(frame->get_frame_number(), frame_process_time, result);

            if(send_image_)
            {
                InairaMLPlugin::LiveImageData live_image = sendImage(frame);
                publish_socket_.send(results, ZMQ_SNDMORE);
                publish_socket_.send(live_image.json_header, ZMQ_SNDMORE);
                publish_socket_.send(frame->get_image_size(), live_image.frame_data_ptr, 0);

            }
            else
            {
                publish_socket_.send(results);
            }
        }
        else
        {
            if(send_image_)
            {
            InairaMLPlugin::LiveImageData live_image = sendImage(frame);
            publish_socket_.send(live_image.json_header, ZMQ_SNDMORE);
            publish_socket_.send(frame->get_image_size(), live_image.frame_data_ptr, 0);
            }
        }
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
            dims[1] = hdr_ptr->frame_height;
            dims[0] = hdr_ptr->frame_width;
            metadata.set_dimensions(dims);

            frame->set_meta_data(metadata);
            frame->set_image_offset(sizeof(Inaira::FrameHeader));
            frame->set_image_size(hdr_ptr->frame_height*hdr_ptr->frame_width * sizeof(uint8_t));
    }

    std::string InairaMLPlugin::sendResults(uint32_t frame_number, uint32_t process_time, std::vector<float> results)
    {
        LOG4CXX_DEBUG(logger_, "Creating Json structure");
        OdinData::JsonDict json;
        json.add("frame_number", frame_number);
        json.add("process_time", process_time);
        json.add("result", results);
        
        std::string json_str = json.str();
        LOG4CXX_DEBUG(logger_, "Json:" << json_str);
        return json_str;
    }

    InairaMLPlugin::LiveImageData InairaMLPlugin::sendImage(boost::shared_ptr<Frame> frame)
    {
        OdinData::JsonDict json;
        const FrameMetaData meta_data = frame->get_meta_data();
        void* frame_data_copy = (void*)frame->get_image_ptr();
        std::vector<uint32_t> dims;
        dims.push_back(meta_data.get_dimensions()[0]);
        dims.push_back(meta_data.get_dimensions()[1]);
        uint32_t frame_num = frame->get_frame_number();


        json.add("frame_num", frame_num);
        json.add("acquisition_id", meta_data.get_acquisition_ID());
        json.add("dtype", get_type_from_enum((DataType)meta_data.get_data_type()));
        json.add("dsize", frame->get_image_size());
        json.add("dataset", meta_data.get_dataset_name());
        json.add("compression", get_compress_from_enum((CompressionType)meta_data.get_compression_type()));

        json.add("shape", dims);

        InairaMLPlugin::LiveImageData image_data;
        image_data.json_header = json.str();
        image_data.frame_data_ptr = frame_data_copy;
        
        return image_data;
        // publish_socket_.send(json_str, ZMQ_SNDMORE);
        // publish_socket_.send(frame->get_image_size(), frame_data_copy, 0);
    }

    void InairaMLPlugin::setSocketAddr(std::string value)
    {
        if(publish_socket_.has_bound_endpoint(value))
        {
            LOG4CXX_WARN(logger_, "Socket already bound to " << value <<". Ignoring");
            return;
        }

        try
        {
            uint32_t linger = 0;
            publish_socket_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
            publish_socket_.unbind(data_socket_addr_.c_str());

            is_bound_ = false;
            data_socket_addr_ = value;

            LOG4CXX_INFO(logger_, "Setting Result Socket Address to " << data_socket_addr_);
            publish_socket_.bind(data_socket_addr_);
            is_bound_ = true;
            LOG4CXX_INFO(logger_, "Socket Bound Successfully.");
        }
        catch(zmq::error_t& e)
        {
            LOG4CXX_ERROR(logger_, "Error binding socket to address " << value << " Error Code: " << e.num());
        }
    }
}