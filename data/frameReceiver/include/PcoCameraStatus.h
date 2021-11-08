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
            }

        private:
            std::string camera_state_name_;
            bool acquiring_;
            unsigned long frames_acquired_;

            friend class PcoCameraLinkController;

    };
}

#endif // PCOCAMERASTATUS_H_