#include "PcoCameraConfiguration.h"

using namespace FrameReceiver;

PcoCameraConfiguration::PcoCameraConfiguration() :
    ConfigContainer(),
    num_frames_(100)
{
    bind_param<unsigned int>("num_frames", num_frames_, "num_frames");
}

