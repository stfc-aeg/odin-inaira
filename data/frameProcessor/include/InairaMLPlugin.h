

#ifndef INCLUDE_InairaMLPLUGIN_H_
#define INCLUDE_InairaMLPLUGIN_H_

#include "InairaProcessorPlugin.h"
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

            static const std::string CONFIG_MODEL_PATH;

            std::string model_path;

            InairaMLCppflow model_;
    };

    /**
     * Registration of this plugin through the ClassLoader.  This macro
     * registers the class without needing to worry about name mangling
     */
    REGISTER(FrameProcessorPlugin, InairaMLPlugin, "InairaMLPlugin");
}





#endif /*INCLUDE_InairaMLPLUGIN_H_*/