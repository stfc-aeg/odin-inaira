#ifndef PCOCAMERACONFIGURATION_H_
#define PCOCAMERACONFIGURATION_H_
#include "ParamContainer.h"

namespace FrameReceiver
{
    namespace Defaults
    {
        const unsigned int default_num_frames = 0;
    }
    class PcoCameraConfiguration : public OdinData::ParamContainer
    {

        public:
            PcoCameraConfiguration() :
                ParamContainer(),
                num_frames_(Defaults::default_num_frames),
                exposure_time_(0),
                delay_time_(0)
            {
                bind_param<unsigned int>(num_frames_, "num_frames");
                bind_param<unsigned int>(exposure_time_, "exposure_time_ms");
                bind_param<unsigned int>(delay_time_, "delay_time_ms");
            }

        private:
            unsigned int num_frames_;
            unsigned int exposure_time_;
            unsigned int delay_time_;

            friend class PcoCameraLinkController;

    };
}

#endif // PCOCAMERACONFIGURATION_H_