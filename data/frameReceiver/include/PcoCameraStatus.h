/*!
 * PcoCameraStatus.h - status parameter container for the PCO camera
 *
 * This class implements a status parameter container for the PCO camera, containing all the
 * status parameters that are captured and reported to controlling clients.
 *
 * Created on: Sep 24, 2021
 *     Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#ifndef PCOCAMERASTATUS_H_
#define PCOCAMERASTATUS_H_

#include "ParamContainer.h"

namespace FrameReceiver
{

    //! PcoCameraStatus - PCO camera status parameter container
    class PcoCameraStatus : public OdinData::ParamContainer
    {

        public:

            //! Default constructor
            //!
            //! This constructor binds all the status parameters in the container, grouping
            //! them as appropriate. This allows them to be accessed with the path-like set/get
            //! mechanism implemented in ParamContainer.

            PcoCameraStatus()
            {
                // Bind overall state parameters
                bind_param<std::string>(camera_state_name_, "camera/state");
                bind_param<bool>(acquiring_, "acquisition/acquiring");
                bind_param<unsigned long>(frames_acquired_, "acquisition/frames_acquired");

                // Bind camera info parameters
                bind_param<std::string>(camera_name_, "camera/info/name");
                bind_param<unsigned int>(camera_type_, "camera/info/type");
                bind_param<unsigned long>(camera_serial_, "camera/info/serial");
            }

        private:
            std::string camera_state_name_;  //!< Name of the current camera state
            bool acquiring_;                 //!< Flag if camera currently acquiring frames
            unsigned long frames_acquired_;  //!< Number of frames acquired in current acqusition

            std::string camera_name_;        //!< Camera name
            unsigned int camera_type_;       //!< Camera product type
            unsigned long camera_serial_;    //!< Camera serial number

            //! Allow the PcoCameraLinkController class direct access to status parameters
            friend class PcoCameraLinkController;

    };
}

#endif // PCOCAMERASTATUS_H_