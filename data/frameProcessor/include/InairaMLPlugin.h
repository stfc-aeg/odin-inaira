

#ifndef INCLUDE_InairaMLPLUGIN_H_
#define INCLUDE_InairaMLPLUGIN_H_

#include "InairaProcessorPlugin.h"
#include "DataBlockFrame.h"
#include "InairaMLCppflow.h"

namespace FrameProcessor
{
    class InairaMLPlugin : public InairaProcessorPlugin
    {
        public:
            InairaMLPlugin();
            virtual ~InairaMLPlugin();

            void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
            void requestConfiguration(OdinData::IpcMessage& reply);
            void status(OdinData::IpcMessage& status);
            bool reset_statistics(void);

        private:
            void process_frame(boost::shared_ptr<Frame> frame);
            void decodeHeader(boost::shared_ptr<Frame> frame);
            void sendResults(uint32_t frame_number, uint32_t process_time, std::vector<float> results);

            void setSocketAddr(std::string value);

            static const std::string CONFIG_MODEL_PATH;
            static const std::string CONFIG_MODEL_INPUT_LAYER;
            static const std::string CONFIG_MODEL_OUTPUT_LAYER;
            static const std::string CONFIG_DECODE_IMG_HEADER;
            static const std::string CONFIG_RESULT_DEST;
            static const std::string CONFIG_SEND_RESULTS;

            std::string model_path;
            bool decode_header;

            InairaMLCppflow model_;
            std::string classes[2];

            std::string data_socket_addr_;
            OdinData::IpcChannel publish_socket_;
            bool is_bound_;
            bool send_results_;

            int32_t avg_process_time;
            int32_t total_process_time;
            int32_t num_processed;
    };

    /**
     * Registration of this plugin through the ClassLoader.  This macro
     * registers the class without needing to worry about name mangling
     */
    REGISTER(FrameProcessorPlugin, InairaMLPlugin, "InairaMLPlugin");
}

#endif /*INCLUDE_InairaMLPLUGIN_H_*/
