#include "PcoCameraConfiguration.h"

using namespace FrameReceiver;

PcoCameraConfiguration::PcoCameraConfiguration() :
    ConfigContainer(),
    num_frames_(Defaults::default_num_frames),
    exposure_time_(0),
    delay_time_(0)
{
    bind_param<unsigned int>("num_frames", num_frames_, "num_frames");
}

