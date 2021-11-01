/*!
 * PcoCameraConfiguration.h - configuration parameter container for the PCO camera
 *
 * This class implements a configuration parameter container for the PCO camera, containing
 * the parameters necessary for operating the camera.
 *
 * Created on: Sep 24, 2021
 *     Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#ifndef PCOCAMERACONFIGURATION_H_
#define PCOCAMERACONFIGURATION_H_

#include "ParamContainer.h"

namespace FrameReceiver
{
    //! Default values for configuration parameters. NB some parameters are synchronised from the
    //! existing state of the camera at startup.
    namespace Defaults
    {
        const unsigned int default_camera_num = 0; //!< Default camera number
        const double default_image_timeout = 10.0; //!< Default image acquisition timeout
        const unsigned int default_num_frames = 0; //!< Default number of frames: 0 = no limit
    }

    //! PcoCameraConfiguration - PCO camera configuration parameter container
    class PcoCameraConfiguration : public OdinData::ParamContainer
    {

        public:

            //! Default constructor
            //!
            //! This constructor initialises all parameters to default values and binds the
            //! parameters in the container, allowing them to accessed via the path-like set/get
            //! mechanism implemented in ParamContainer.

            PcoCameraConfiguration() :
                ParamContainer(),
                camera_num_(Defaults::default_camera_num),
                image_timeout_(Defaults::default_image_timeout),
                num_frames_(Defaults::default_num_frames),
                exposure_time_(0.0),
                frame_rate_(0.0)
            {
                // Bind parameters in the container
                bind_param<unsigned int>(camera_num_, "camera_num");
                bind_param<double>(image_timeout_, "image_timeout");
                bind_param<unsigned int>(num_frames_, "num_frames");
                bind_param<double>(exposure_time_, "exposure_time");
                bind_param<double>(frame_rate_, "frame_rate");
            }

        private:

            unsigned int camera_num_; //!< Camera number as enumerated by driver
            double image_timeout_;    //!< Image acquisition timeout in seconds
            unsigned int num_frames_; //!< Number of frames to acquire, 0 = no limit
            double exposure_time_;    //!< Exposure time in seconds
            double frame_rate_;       //!< Frame rate in Hertz

            //! Allow the PcoCameraLinkController class direct access to config parameters
            friend class PcoCameraLinkController;

    };
}

#endif // PCOCAMERACONFIGURATION_H_