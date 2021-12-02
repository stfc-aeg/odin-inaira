/*
 * PcoCameraLinkController.h - controller class for the PCO camera integration into odin-data
 *
 * Created on: 20 July 2021
 *     Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#ifndef INCLUDE_PCOCAMERALINKCONTROLLER_H_
#define INCLUDE_PCOCAMERALINKCONTROLLER_H_

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include <log4cxx/logger.h>
using namespace log4cxx;
using namespace log4cxx::helpers;
#include <DebugLevelLogger.h>
#include <IpcMessage.h>

#include "PcoCameraStateMachine.h"
#include "PcoCameraConfiguration.h"
#include "PcoCameraDelayExposureConfig.h"
#include "PcoCameraStatus.h"

#include "Cpco_com.h"
#include "Cpco_grab_clhs.h"

using namespace OdinData;

namespace FrameReceiver
{

  class PcoCameraLinkFrameDecoder; // Forward declaration of the decoder class

  const DWORD default_pco_error = -1;
  const WORD recording_state_stopped = 0;
  const WORD recording_state_running = 1;
  class PcoCameraLinkController
  {
  public:

    //! Constructor for the controller taking a pointer to the decoder as a argument
    PcoCameraLinkController(PcoCameraLinkFrameDecoder* decoder);

    //! Destructor for the controller class
    ~PcoCameraLinkController();

    //! Returns the camera image width in number of pixels
    uint32_t get_image_width(void);

    //! Returns the camera image height in number of pixels
    uint32_t get_image_height(void);

    //! Returns the camera image data type required for the frame header parameters
    int get_image_data_type(void);

    //! Returns the camera image size in bytes
    std::size_t get_image_size(void);

    //! Executes a camera control command
    void execute_command(std::string& command);

    //! Updates the configuration of the camera from a parameter document
    void update_configuration(ParamContainer::Document& params);

    //! Gets the current configuration of the camera encoded into a parameter document
    void get_configuration(ParamContainer::Document& params, const std::string param_prefix);

    //! Gets the current status of the camera encoded into a parameter document
    void get_status(ParamContainer::Document& params, const std::string param_prefix);

    //! Disconnects from the camera
    bool disconnect(bool reset_error_status = false);

    //! Connects to the camera
    bool connect(void);

    //! Arms the camera, preparing for recording
    bool arm(void);

    //! Disarms the camera
    bool disarm(void);

    //! Starts the camera recording, allowing images to be acquired
    bool start_recording(void);

    //! Stops the camera recording
    bool stop_recording(void);

    //! Runs the camera control loop service
    void run_camera_service(void);

    //! Returns the name of the current camera state
    std::string camera_state_name(void);

    //! Returns true if the camera is currently recording
    bool camera_recording(void) { return camera_recording_; }

  private:

    //! Acquires an image from camera into an image buffer
    bool acquire_image(void* image_buffer, int timeout);

    //! Calculates the image number from the timestamp in the first pixel data
    int image_nr_from_timestamp(void *image_buffer, int shift);

    //! Checks camera error codes, setting camera error status and emitting error messages
    bool check_pco_error(const std::string message, DWORD pco_error = default_pco_error);

    //! Returns a readable error string for a camera error code
    std::string pco_error_text(DWORD pco_error);

    LoggerPtr logger_;                                   //!< Pointer to the message logger
    PcoCameraLinkFrameDecoder* decoder_;                 //!< Pointer to the frame decoder instance
    PcoCameraState camera_state_;                        //!< Camera state machine instance
    PcoCameraConfiguration camera_config_;               //!< Camera config parameter container
    PcoCameraStatus camera_status_;                      //!< Camera status parameter container
    PcoCameraDelayExposure camera_delay_exp_;            //!< Camera delay/exposure instance
    boost::shared_ptr<boost::thread> controller_thread_; //!< Pointer to controller service thread

    boost::scoped_ptr<CPco_com> camera_;        //!< Scoped pointer to camera instance
    boost::scoped_ptr<CPco_grab_clhs> grabber_; //!< Scoped pointer to frame grabber instance

    bool camera_opened_;             //!< Indicates if camera connection is opened
    bool grabber_opened_;            //!< Indicates if frame grabber connection is opened
    volatile bool camera_recording_; //!< Indicates if camera is recording

    uint32_t image_width_;        //!< Image width in pixels
    uint32_t image_height_;       //!< Image height in pixels
    uint32_t image_pixel_size_;   //!< Image pixel size in bytes
    std::size_t image_data_type_; //!< Image data type for frame header

  };
}
#endif // INCLUDE_PCOCAMERALINKCONTROLLER_H_
