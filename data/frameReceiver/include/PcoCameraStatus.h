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

    const unsigned long error_code_none = 0;
    const std::string error_message_none = "no error";

    //! PcoCameraStatus - PCO camera status parameter container
    class PcoCameraStatus : public OdinData::ParamContainer
    {

        public:

            //! Default constructor
            //!
            //! This constructor binds all the status parameters in the container, grouping
            //! them as appropriate. This allows them to be accessed with the path-like set/get
            //! mechanism implemented in ParamContainer.

            PcoCameraStatus() :
                camera_state_name_("unknown"),
                acquiring_(false),
                frames_acquired_(0),
                error_code_(error_code_none),
                error_message_(error_message_none),
                camera_name_("unknown"),
                camera_type_(0),
                camera_serial_(0)
            {
                // Bind overall state parameters
                bind_param<std::string>(camera_state_name_, "camera/state");
                bind_param<bool>(acquiring_, "acquisition/acquiring");
                bind_param<unsigned long>(frames_acquired_, "acquisition/frames_acquired");

                // Bind error status parameters
                bind_param<unsigned long>(error_code_, "camera/error/code");
                bind_param<std::string>(error_message_, "camera/error/message");

                // Bind camera info parameters
                bind_param<std::string>(camera_name_, "camera/info/name");
                bind_param<unsigned int>(camera_type_, "camera/info/type");
                bind_param<unsigned long>(camera_serial_, "camera/info/serial");
            }

            //! Resets the error status parameters
            //!
            //! This is a convenience method to allow the error status parameters to be reset to
            //! their default "no error" state.

            void reset_error_status(void)
            {
                error_code_ = error_code_none;
                error_message_ = error_message_none;
            }

        private:
            std::string camera_state_name_;  //!< Name of the current camera state
            bool acquiring_;                 //!< Flag if camera currently acquiring frames
            unsigned long frames_acquired_;  //!< Number of frames acquired in current acqusition

            unsigned long error_code_;       //!< PCO camera error code
            std::string error_message_;      //!< Camera error message

            std::string camera_name_;        //!< Camera name
            unsigned int camera_type_;       //!< Camera product type
            unsigned long camera_serial_;    //!< Camera serial number

            //! Allow the PcoCameraLinkController class direct access to status parameters
            friend class PcoCameraLinkController;

    };
}

#endif // PCOCAMERASTATUS_H_