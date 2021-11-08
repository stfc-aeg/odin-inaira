#ifndef INCLUDE_PCOCAMERAPROCESSPLUGIN_H_
#define INCLUDE_PCOCAMERAPROCESSPLUGIN_H_

#include "InairaProcessorPlugin.h"

namespace FrameProcessor
{
    class PcoCameraProcessPlugin : public InairaProcessorPlugin
    {
        public:
            PcoCameraProcessPlugin();
            virtual ~PcoCameraProcessPlugin();

            void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
            void requestConfiguration(OdinData::IpcMessage& reply);
            void status(OdinData::IpcMessage& status);
            bool reset_statistics(void);

        private:
            void process_frame(boost::shared_ptr<Frame> frame);
            void decodeHeader(boost::shared_ptr<Frame> frame);

    };

    /*
     * Registration of this plugin through the ClassLoader.  This macro
     * registers the class without needing to worry about name mangling
     */
    REGISTER(FrameProcessorPlugin, PcoCameraProcessPlugin, "PcoCameraProcessPlugin");
}

#endif /* INCLUDE_PCOCAMERAPROCESSPLUGIN_H_ */