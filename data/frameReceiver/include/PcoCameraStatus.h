#ifndef PCOCAMERASTATUS_H_
#define PCOCAMERASTATUS_H_

#include "ParamContainer.h"

namespace FrameReceiver
{

    class PcoCameraStatus : public OdinData::ParamContainer
    {

        public:
            PcoCameraStatus()
            {
                bind_param<std::string>(camera_state_name_, "camera/state");
                bind_param<bool>(acquiring_, "acquisition/acquiring");
                bind_param<unsigned long>(frames_acquired_, "acquisition/frames_acquired");

                bind_param<std::string>(camera_name_, "camera/info/name");
                bind_param<unsigned int>(camera_type_, "camera/info/type");
                bind_param<unsigned long>(camera_serial_, "camera/info/serial");
            }

        private:
            std::string camera_state_name_;
            bool acquiring_;
            unsigned long frames_acquired_;

            std::string camera_name_;
            unsigned int camera_type_;
            unsigned long camera_serial_;

            friend class PcoCameraLinkController;

    };
}

#endif // PCOCAMERASTATUS_H_