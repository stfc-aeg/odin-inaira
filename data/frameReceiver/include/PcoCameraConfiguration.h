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
                exposure_time_(0.0),
                frame_rate_(0.0)
            {
                bind_param<unsigned int>(num_frames_, "num_frames");
                bind_param<double>(exposure_time_, "exposure_time");
                bind_param<double>(frame_rate_, "frame_rate");
            }

        private:
            unsigned int num_frames_;
            double exposure_time_;
            double frame_rate_;
            friend class PcoCameraLinkController;

    };
}

#endif // PCOCAMERACONFIGURATION_H_