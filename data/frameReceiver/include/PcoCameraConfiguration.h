#include "ConfigContainer.h"

namespace FrameReceiver
{

    class PcoCameraConfiguration : public ConfigContainer
    {

        public:
            PcoCameraConfiguration();

        private:
            unsigned int num_frames_;

            friend class PcoCameraLinkController;

    };
}