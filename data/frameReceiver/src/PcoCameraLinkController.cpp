/*
 * PcoCameraLinkControllercpp - controller class for the PCO camera integration into odin-data
 *
 * This class implements a controller for the PCO camera. The controller is responsible for the
 * control of the PCO camera within the frame decoder and runs a service thread that implements
 * the image acquisition loop. The controller maintains configuration and status parameters that
 * are accessible to the frame decoder and manages state transitions under command of the decoder
 * client connection.
 *
 * Created on: 20 July 2021
 *     Author: Tim Nicholls, STFC Detector Systems Group
 */

#include <math.h>

#include "PcoCameraLinkController.h"
#include "PcoCameraStateMachine.h"
#include "PcoCameraLinkFrameDecoder.h"
#include "file12.h"
#include "IpcMessage.h"
#include "InairaDefinitions.h"

using namespace FrameReceiver;

//! PcoCameraLinkController constructor
//!
//! This constructor initialises the camera controller, setting up the initial state and
//! configuration of the camera. In order for the decoder to report the image dimensions during
//! frame receiver configuration, the camera is connected, armed and started so that the image
//! width and height can be obtained from the frame grabber.
//!
//! \param decoder - pointer to the parent frame decoder

PcoCameraLinkController::PcoCameraLinkController(PcoCameraLinkFrameDecoder* decoder) :
    logger_(Logger::getLogger("FR.PcoCLController")),
    decoder_(decoder),
    camera_state_(this),
    camera_opened_(false),
    grabber_opened_(false),
    camera_recording_(false),
    image_width_(0),
    image_height_(0),
    image_data_type_(2) // TODO Get this from common header?
{
    DWORD pco_error;

    LOG4CXX_INFO(logger_, "Initialising camera system");

    camera_status_.acquiring_ = false;
    camera_status_.frames_acquired_ = 0;

    // Initialise the camera state machine and connect to the camera to synchronise
    // configuration settings
    camera_state_.initiate();
    camera_state_.execute_command(PcoCameraState::CommandConnect);

    // Arm and start the camera recording so that the image size can be determined from the
    // frame grabber
    camera_state_.execute_command(PcoCameraState::CommandArm);
    camera_state_.execute_command(PcoCameraState::CommandStartRecording);

    pco_error = grabber_->Get_actual_size(&image_width_, &image_height_, NULL);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to get actual size from grabber with error code 0x"
            << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }
    LOG4CXX_INFO(logger_, "Grabber reports actual size: width: "
        << image_width_ << " height: " << image_height_);

    // Stop the camera recording
    camera_state_.execute_command(PcoCameraState::CommandStopRecording);
}

//! Destructor for the controller class
//!
//! This is the PcoCameraLinkController destructor, which ensures that the connection to the
//! camera and grabber is cleaned up corectly on destruction, by calling the diconnect method

PcoCameraLinkController::~PcoCameraLinkController()
{
    this->disconnect();
}


//! Executes a camera control command
//!
//! This method exectures a camera control command, passing the specified command to the camera
//! state machine to trigger the appropriate action.
//!
//! \param command - string command name to execute

void PcoCameraLinkController::execute_command(std::string& command)
{
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Controller executing command " << command);
    try
    {
        camera_state_.execute_command(command);
    }
    catch (PcoCameraStateException& e)
    {
        LOG4CXX_ERROR(logger_, "Failed to execute " << command << " command: " << e.what());
    }
    LOG4CXX_INFO(logger_, "Camera state is now: " << camera_state_.current_state_name());
}

//! Updates the configuration of the camera from a parameter document
//!
//! This method updates the configuration parameters of the controller from the specifed parameter
//! document (i.e. JSON parameter payload from an IpcMessage command received by the decoder). The
//! configuration parameter container is updated and then new delay/exposure parameters derived as
//! appropriate, triggering an update to the camera setttings.
//!
//! \param params - JSON parameter document to update configuration from

void PcoCameraLinkController::update_configuration(ParamContainer::Document& params)
{
    // Update the camera configuration with the specified parameters
    camera_config_.update(params);

    // Create a new delay/exposure configuration based on updated settings
    PcoCameraDelayExposure new_delay_exp(camera_config_.exposure_time_, camera_config_.frame_rate_);

    // If this delay/exposure configuration differs from the current settings, update the camera
    // parameters accordingly. Note that if the camera is recording this will cause an immediate
    // change in the frame rate and exposure time. If not, the camera must be re-armed to commit
    // those settings.
    // TODO - if camera is not recording, flag that a re-arm is needed or disarm the camera if
    // necessary
    if (new_delay_exp != camera_delay_exp_)
    {
        LOG4CXX_DEBUG_LEVEL(2, logger_, "Updating camera delay and exposure settings");

        DWORD pco_error;
        bool updated = true;

        // Set the delay and exposure timebases
        pco_error = camera_->PCO_SetTimebase(
            new_delay_exp.delay_timebase_, new_delay_exp.exposure_timebase_
        );
        if (pco_error != PCO_NOERROR)
        {
            LOG4CXX_ERROR(logger_, "Failed to set timebase with error code 0x"
                << std::hex << pco_error << std::dec);
                updated = false;
        }

        // Set the delay and exposure times
        pco_error = camera_->PCO_SetDelayExposure(
            new_delay_exp.delay_time_, new_delay_exp.exposure_time_
        );
        if (pco_error != PCO_NOERROR)
        {
            LOG4CXX_ERROR(logger_, "Failed to set camera delay and exposure with error code 0x"
                << std::hex << pco_error << std::dec);
                updated = false;
        }
        if (updated)
        {
            camera_delay_exp_ = new_delay_exp;
        }
    }
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Camera config num_frames is now " << camera_config_.num_frames_)
}

//! Gets the current configuration of the camera.
//!
//! This method gets the current configuration of the camera from the configuration parameter
//! container, encoding it into the specified JSON document at the specified path.
//!
//! \param params - JSON document to encode config params into
//! \param param_prefix - string prefix to encode parameters with

void PcoCameraLinkController::get_configuration(ParamContainer::Document& params,
    const std::string param_prefix)
{
    camera_config_.encode(params, param_prefix);
}

//! Gets the current status of the camera.
//!
//! This method gets the current status of the camera from the status parameter container,
//! encoding it into the specified JSON document at the specified path.
//!
//! \param params - JSON document to encode status params into
//! \param param_prefix - string prefix to encode parameters with

void PcoCameraLinkController::get_status(ParamContainer::Document& params,
    const std::string param_prefix)
{
    camera_status_.camera_state_name_ = camera_state_.current_state_name();
    camera_status_.encode(params, param_prefix);
}

//! Disconnects from the camera.
//!
//! This method disconnects from the PCO camera and grabber and is invoked by the camera state
//! machine on receipt of the appropriate command.

void PcoCameraLinkController::disconnect(void)
{
    LOG4CXX_INFO(logger_, "Disconnecting camera");

    // Stop the camera recording if necessary
    if (camera_ && camera_recording_) {
        LOG4CXX_DEBUG_LEVEL(2, logger_, "Disconnect: setting camera recording state to stop");
        this->stop_recording();
    }

    // Close the grabber connection and delete the instance
    if (grabber_ && grabber_opened_) {
        LOG4CXX_DEBUG_LEVEL(2, logger_, "Disconnect: closing PCO grabber");
        grabber_->Close_Grabber();
        grabber_opened_ = false;
        grabber_.reset();
    }

    // Close the camera connection and delete the instance
    if (camera_ && camera_opened_) {
        LOG4CXX_DEBUG_LEVEL(2, logger_, "Disconnect: closing PCO camera");
        camera_->Close_Cam();
        camera_opened_ = false;
        camera_.reset();
    }
}

//! Connects to the camera.
//!
//! This method connects to the PCO camera and initialises the state of the controller. Camera and
//! grabber instances are created and opened, and various configuration and status values read
//! back from the system to synchronise the controller with the camera.

void PcoCameraLinkController::connect(void)
{

    DWORD pco_error;
    WORD camera_type;
    DWORD camera_serial;
    SC2_Camera_Description_Response camera_description;
    char camera_infostr[100];

    LOG4CXX_INFO(logger_, "Connecting camera");

    // Create a new PCO camera instance
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Creating PCO camera instance")
    camera_.reset(new CPco_com_clhs());
    if (camera_ == NULL)
    {
        LOG4CXX_ERROR(logger_, "Failed to create PCO camera object");
        this->disconnect();
        return;

    }

    // Open the camera connection
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Opening PCO camera " << camera_config_.camera_num_);
    pco_error = camera_->Open_Cam(camera_config_.camera_num_);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to open PCO camera with error code 0x"
            << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }
    else
    {
        camera_opened_ = true;
    }

    // Create a new grabber instance
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Creating PCO grabber instance")
    grabber_.reset(new CPco_grab_clhs((CPco_com_clhs*)(camera_.get())));
    if (grabber_ == NULL)
    {
        LOG4CXX_ERROR(logger_, "Failed to create PCO frame grabber object");
        this->disconnect();
        return;
    }

    // Open the grabber connection
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Opening PCO grabber " << camera_config_.camera_num_);
    pco_error = grabber_->Open_Grabber(camera_config_.camera_num_);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to open PCO grabber with error code 0x"
            << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }
    else
    {
        grabber_opened_ = true;
    }

    // Set the grabber image acquisition timeout
    int grabber_timeout_ms_ = static_cast<int>(camera_config_.image_timeout_ * 1000);
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Setting grabber image timeout to "
        << grabber_timeout_ms_ << "ms"
    );
    pco_error = grabber_->Set_Grabber_Timeout(grabber_timeout_ms_);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to set PCO grabber timeeout with error code 0x"
            << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }


    // Read the camera type and serial number
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Getting camera type and serial number");
    pco_error = camera_->PCO_GetCameraType(&camera_type, &camera_serial);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to get camera type with error code 0x"
            << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }

    // Read the camera descriptor to determine the dynamic resolution and pixel data size
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Getting camera descriptor");
    pco_error = camera_->PCO_GetCameraDescriptor(&camera_description);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to camera descriptor with error code 0x"
            << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }
    image_pixel_size_ = floorl((camera_description.wDynResDESC-1)/8) + 1;
    LOG4CXX_INFO(logger_, "Camera descriptor reports dynamic resolution: " <<
        camera_description.wDynResDESC << " pixel size: " << image_pixel_size_ << " bytes"
    );

    // Read the camera information string o get the camera name
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Getting camera information string");
    pco_error = camera_->PCO_GetInfo(1, camera_infostr, sizeof(camera_infostr));
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to camera info with error code 0x"
            << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }

    // Populate the camera status container with the appropriate information
    camera_status_.camera_name_ = std::string(camera_infostr);
    camera_status_.camera_type_ = camera_type;
    camera_status_.camera_serial_ = camera_serial;

    LOG4CXX_INFO(logger_, "Connected to PCO camera with name: '" << camera_infostr
        << "' type: 0x" << std::hex << camera_type << std::dec
        << " serial number: " << camera_serial
    );

    // Read the camera delay and exposure settings
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Getting camera delay and exposure times");
    pco_error = camera_->PCO_GetDelayExposure(
        &(camera_delay_exp_.delay_time_), &(camera_delay_exp_.exposure_time_)
    );
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to get delay and exposure times with error code 0x"
             << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }

    // Read the camera delay and exposure timebase settings
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Getting camera delay and exposure timebase");
    pco_error = camera_->PCO_GetTimebase(
        &(camera_delay_exp_.delay_timebase_), &(camera_delay_exp_.exposure_timebase_)
    );
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to get delay and exposure timebase with error code 0x"
             << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }

    // Update the camera exposure and frame rate in the configuration container accordingly
    camera_config_.exposure_time_ = camera_delay_exp_.exposure_time();
    camera_config_.frame_rate_ = camera_delay_exp_.frame_rate();

    LOG4CXX_INFO(logger_, "Camera reports exposure time: "
        << camera_delay_exp_.exposure_time_ << camera_delay_exp_.exposure_timebase_unit()
        << " delay time: " << camera_delay_exp_.delay_time_ << camera_delay_exp_.delay_timebase_unit()
        << " frame rate: " << camera_config_.frame_rate_ << "Hz"
    );

}

//! Arms the camera, preparing for recording
//!
//! This method arms the PCO camera and is invoked by the camera state machine on receipt of the
//! appropriate command. Arming is necessary to commit new settings to the camera for image
//! recording.

void PcoCameraLinkController::arm(void)
{
    DWORD pco_error;

    // Arm the camera
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Arming camera");
    pco_error = camera_->PCO_ArmCamera();
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to arm camera with error code 0x"
            << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }

    // Post-arm the grabber
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Post-arming grabber");
    pco_error = grabber_->PostArm();
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to post-arm grabber with error code 0x"
            << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }
}

//! Disarms the camera
//!
//! This method diarms the PCO camera and is invoked by the camera state machine on receipt of the
//! appropriate command. Disarming the camera is a logic-only operation within the
//! state machine - no camera operations are necessary.

void PcoCameraLinkController::disarm(void)
{
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Disarming camera");
}

//! Starts the camera recording, allowing images to be acquired.
//!
//! This method sets the camera recording state to true, allowing images to be acquired, and is
//! invoked by the camera state machine on receipt of the appropriate command.

void PcoCameraLinkController::start_recording(void)
{
    DWORD pco_error;
    DWORD exp_time, delay_time;

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Setting camera recording state to running");

    pco_error = camera_->PCO_SetRecordingState(1);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to set camera recoding state to running with error code 0x"
            << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }

    camera_recording_ = true;
}

//! Stops the camera recording.
//!
//! This method sets the camera recording state to false, stopping images acquisition, and is
//! invoked by the camera state machine on receipt of the appropriate command.

void PcoCameraLinkController::stop_recording(void)
{
    DWORD pco_error;

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Setting camera recording state to stopped");
    pco_error = camera_->PCO_SetRecordingState(0);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to set camera recoding state to stopped with error code 0x"
            << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }
    camera_recording_ = false;
}

//! Runs the camera control loop service
//!
//! This method runs the camera control loop service and is invoked in a separate thread by the
//! frame decoder instance. The service is responsible for image acquisiton when the camera is
//! recording and for determining if the current acqusition has completed the appropriate number
//! of frames. When the camera is not recording, the service runs in a idle tick.

void PcoCameraLinkController::run_camera_service(void)
{

    DWORD pco_error;

    // Initialise acquistion status parameters
    camera_status_.acquiring_ = false;
    camera_status_.frames_acquired_ = 0;

    // Calculate intial image timeout in milliseconds from configuration
    int image_timeout_ms = static_cast<int>(camera_config_.image_timeout_);

    // Loop while the decoder indicates the camera service should run
    while (decoder_->run_camera_service_thread())
    {
        // If the camera is recording, handle image acquisition
        if (camera_recording_)
        {
            // If the camera state is now set to recording but we have not yet enabled acquisition
            // in the grabber, do so now
            if (!camera_status_.acquiring_)
            {

                // Recalculate image timeout in milliseconds
                image_timeout_ms = static_cast<int>(camera_config_.image_timeout_ * 1000);

                // Start acquisition on the grabber
                pco_error = grabber_->Start_Acquire();
                if (pco_error != PCO_NOERROR)
                {
                    LOG4CXX_ERROR(logger_, "Failed to start frame grabber acquisition error code 0x"
                        << std::hex << pco_error << std::dec);
                }

                std::stringstream ss;
                if (camera_config_.num_frames_)
                {
                    ss << camera_config_.num_frames_;
                }
                else
                {
                    ss << "unlimited";
                }
                LOG4CXX_DEBUG_LEVEL(1, logger_,
                    "Camera controller now acquiring " << ss.str() << " frames");
                camera_status_.acquiring_ = true;
                camera_status_.frames_acquired_ = 0;
            }

            // Request an empty buffer from the decoder and acquire an image into it
            int buffer_id;
            void* buffer_addr;
            if (decoder_->get_empty_buffer(buffer_id, buffer_addr))
            {
                LOG4CXX_DEBUG_LEVEL(2, logger_, "Decoder got empty buffer id " << buffer_id
                    << " at addr 0x" << std::hex << (unsigned long)buffer_addr << std::dec );

                // Get a pointer to the image location in the buffer, o.e. offset from the start
                // of the buffer by the header size
                void* image_buffer = reinterpret_cast<void*>(
                    reinterpret_cast<uint8_t*>(buffer_addr) + decoder_->get_frame_header_size()
                );

                // Acquire an image from the camera into the buffer
                if (this->acquire_image(image_buffer, image_timeout_ms))
                {
                    // Populate the fields of the frame header
                    Inaira::FrameHeader* frame_hdr =
                        reinterpret_cast<Inaira::FrameHeader*>(buffer_addr);
                    frame_hdr->frame_number = camera_status_.frames_acquired_;
                    frame_hdr->frame_width = image_width_;
                    frame_hdr->frame_height = image_height_;
                    frame_hdr->frame_data_type = image_data_type_;
                    frame_hdr->frame_size = get_image_size();

                    // Notify the frame receiver main control thread that the frame is ready to
                    // be processed downstream
                    decoder_->notify_frame_ready(buffer_id, camera_status_.frames_acquired_);
                    camera_status_.frames_acquired_++;
                }
            }
            else
            {
                // The logic of handling when no buffers are available needs improving with
                // retry attempts, but for now simply report the failure as a warning
                LOG4CXX_WARN(logger_, "Failed to get empty buffer from queue");
            }

            // If the current configuration specifies a number of frames to acquire, check if
            // that has been reached and, if so, stop the acquisition
            if (camera_config_.num_frames_ &&
                (camera_status_.frames_acquired_ >= camera_config_.num_frames_))
            {
                pco_error = grabber_->Stop_Acquire();
                if (pco_error != PCO_NOERROR)
                {
                    LOG4CXX_ERROR(logger_,
                        "Failed to stop frame grabber acquisition with error code 0x"
                        << std::hex << pco_error << std::dec);
                }
                LOG4CXX_INFO(logger_,
                    "Camera controller completed acquisition of "
                    << camera_status_.frames_acquired_ << " frames"
                );
                camera_status_.acquiring_ = false;
                camera_state_.execute_command(PcoCameraState::CommandType::CommandStopRecording);
            }
        }
        else
        {
            // Idle loop for when the camera is not recording. If acquisition has just completed
            // report the number of frames acquired
            if (camera_status_.acquiring_)
            {
                LOG4CXX_DEBUG_LEVEL(1, logger_, "Camera controller finished acquiring after "
                    << camera_status_.frames_acquired_ << " frames");
                camera_status_.acquiring_ = false;
            }
            usleep(1000);
        }
    }
}

//! Returns the name of the current camera state
//!
//! This method returns the readable name of the current state of the camera state machine
//!
//! \return current state name as a string

std::string PcoCameraLinkController::camera_state_name(void)
{
    return camera_state_.current_state_name();
}

//! Returns the camera image width in number of pixels
//!
//! This method returns the width of the camera image as an integer number of pixels
//!
//! \return the camera image width in pixels as an unsigned integer

uint32_t PcoCameraLinkController::get_image_width(void)
{
    return image_width_;
}

//! Returns the camera image height in number of pixels.
//!
//! This method returns the height of the camera image as an integer number of pixels.
//!
//! \return the camera image height in pixels as an unsigned integer

uint32_t PcoCameraLinkController::get_image_height(void)
{
    return image_height_;
}

//! Returns the camera image data type required for the frame header parameters.
//!
//! This method returns the image data type, as required for frame header parameters.
//!
//! \return image data type as in integer

int PcoCameraLinkController::get_image_data_type(void)
{
    return image_data_type_;
}

//! Returns the camera image size in bytes
//!
//! This method calculates and returns the size of the camera image in bytes, based on the
//! image dimensions and pixel data size read from the camera during initialisation
//!
//! \return camera image size in bytes

std::size_t PcoCameraLinkController::get_image_size(void)
{
    std::size_t image_size = image_width_ * image_height_ * image_pixel_size_;
    return image_size;
}

// Private methods

//! Acquires an image from camera into an image buffer
//!
//! This method acquires an image from the camera, reading it into the specified image buffer. THe
//! return value indicates if the acquisition succeeded.
//!
//! \param image_buffer - void pointer to the location in the buffer to receive the image
//! \param timeout - image acquisition timeout in milliseconds
//! \return boolean flag indicating if the acquisition succeeded

bool PcoCameraLinkController::acquire_image(void* image_buffer, int timeout)
{
    bool acquire_ok = true;
    DWORD pco_error;

        //pco_error = grabber_->Acquire_Image(reinterpret_cast<WORD*>(image_buffer));

        pco_error = grabber_->Wait_For_Next_Image(reinterpret_cast<WORD*>(image_buffer), timeout);

        if (pco_error != PCO_NOERROR)
        {
            LOG4CXX_ERROR(logger_, "Failed to acquire an image with error code 0x"
                << std::hex << pco_error << std::dec);
            acquire_ok = false;
        }
        else
        {
            int image_num = this->image_nr_from_timestamp(image_buffer, 0);
            LOG4CXX_INFO(logger_, "Image acquisition completed OK with image number: " << image_num);
        }
    return acquire_ok;
}

//! Calculates the image number from the timestamp in the first pixel data
//!
//! This method, taken from an example in the PCO SDK demo applications, calculates the camera
//! image number from the BCD coded values stored in the first four pixels of the image.
//!
//! \param image_buffer - pointer to image buffer in memory
//! \param shift - number of bits to right-shift each BCD pixel value
//! \return image number as an integer

int PcoCameraLinkController::image_nr_from_timestamp(void *image_buffer, int shift)
{
    int image_num = 0;
    unsigned short *pixel_ptr = (unsigned short *)(image_buffer);

    for (int bcd_mult = 100*100*100; bcd_mult > 0; bcd_mult /= 100)
    {
        *pixel_ptr >>= shift;
        image_num += (((*pixel_ptr & 0x00F0)>>4)*10 + (*pixel_ptr & 0x000F)) * bcd_mult ;
        pixel_ptr++;
    }
    return image_num;
}
