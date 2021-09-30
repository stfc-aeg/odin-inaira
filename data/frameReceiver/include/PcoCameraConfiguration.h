#include "ConfigContainer.h"

namespace FrameReceiver
{

    namespace Defaults
    {
        const unsigned int default_num_frames = 0;
    }
    class PcoCameraConfiguration : public ConfigContainer
    {

        public:
            PcoCameraConfiguration();

        private:
            unsigned int num_frames_;
            int exposure_time_;
            int delay_time_;
            friend class PcoCameraLinkController;

    };
}