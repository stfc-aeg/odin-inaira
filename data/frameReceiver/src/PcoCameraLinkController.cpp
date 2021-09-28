#include <math.h>

#include "PcoCameraLinkController.h"
#include "PcoCameraStateMachine.h"
#include "PcoCameraLinkFrameDecoder.h"
#include "file12.h"
#include "IpcMessage.h"
#include "InairaDefinitions.h"

using namespace FrameReceiver;

PcoCameraLinkController::PcoCameraLinkController(PcoCameraLinkFrameDecoder* decoder) :
    logger_(Logger::getLogger("FR.PcoCLController")),
    decoder_(decoder),
    camera_state_(this),
    camera_opened_(false),
    grabber_opened_(false),
    camera_running_(false),
    camera_num_(0),
    grabber_timeout_ms_(10000),
    image_width_(0),
    image_height_(0)
{
    DWORD pco_error;

    DWORD exp_time, delay_time;

    LOG4CXX_INFO(logger_, "Initialising camera system");

    std::cout << "Config has num frames " << camera_config_.num_frames_ << std::endl;

    camera_state_.initiate();
    camera_state_.execute_command(PcoCameraState::CommandConnect);
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

    pco_error = camera_->PCO_GetDelayExposure(&delay_time, &exp_time);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to get delay and exposure times with error code 0x"
             << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }
    LOG4CXX_INFO(logger_, "Camera reports exposure time: "
        << exp_time << "ms delay time: " << delay_time << "ms");

    camera_state_.execute_command(PcoCameraState::CommandStopRecording);
}

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

void PcoCameraLinkController::disconnect(void)
{
    if (camera_)
    {
        LOG4CXX_INFO(logger_, "Disconnecting camera");

        if (camera_running_) {
            LOG4CXX_DEBUG_LEVEL(2, logger_, "Disconnect: setting camera recording state to stop");
            this->stop_recording();
        }

        if (grabber_ && grabber_opened_) {
            LOG4CXX_DEBUG_LEVEL(2, logger_, "Disconnect: closing PCO grabber");
            grabber_->Close_Grabber();
            grabber_opened_ = false;
        }

        if (camera_opened_) {
            LOG4CXX_DEBUG_LEVEL(2, logger_, "Disconnect: closing PCO camera");
            camera_->Close_Cam();
            camera_opened_ = false;
        }
    }
}

void PcoCameraLinkController::connect(void)
{

    DWORD pco_error;
    WORD camera_type;
    DWORD camera_serial;
    SC2_Camera_Description_Response camera_description;
    char camera_infostr[100];

    LOG4CXX_INFO(logger_, "Connecting camera");

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Creating PCO camera object")
    camera_.reset(new CPco_com_clhs());
    if (camera_ == NULL)
    {
        LOG4CXX_ERROR(logger_, "Failed to create PCO camera object");
        this->disconnect();
        return;

    }

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Opening PCO camera " << camera_num_);
    pco_error = camera_->Open_Cam(camera_num_);
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

    pco_error = camera_->PCO_GetCameraType(&camera_type, &camera_serial);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to get camera type with error code 0x"
            << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Creating PCO grabber object")
    grabber_.reset(new CPco_grab_clhs((CPco_com_clhs*)(camera_.get())));
    if (grabber_ == NULL)
    {
        LOG4CXX_ERROR(logger_, "Failed to create PCO frame grabber object");
        this->disconnect();
        return;
    }

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Opening PCO grabber " << camera_num_);
    pco_error = grabber_->Open_Grabber(camera_num_);
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

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Setting grabber timeout to " << grabber_timeout_ms_ << "ms");
    pco_error = grabber_->Set_Grabber_Timeout(grabber_timeout_ms_);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to set PCO grabber timeeout with error code 0x"
            << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Getting camera info");
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


    pco_error = camera_->PCO_GetInfo(1, camera_infostr, sizeof(camera_infostr));
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to camera info with error code 0x"
            << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }

    LOG4CXX_INFO(logger_, "Connected to PCO camera with name: '" << camera_infostr
        << "' type: 0x" << std::hex << camera_type << std::dec
        << " serial number: " << camera_serial
    );
}

void PcoCameraLinkController::arm(void)
{
    DWORD pco_error;

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Arming camera");
    pco_error = camera_->PCO_ArmCamera();
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to arm camera with error code 0x"
            << std::hex << pco_error << std::dec);
        this->disconnect();
        return;
    }

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

void PcoCameraLinkController::disarm(void)
{
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Disarming camera");
}

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

    camera_running_ = true;
}

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
    camera_running_ = false;
}

std::string PcoCameraLinkController::camera_state_name(void)
{
    return camera_state_.current_state_name();
}

PcoCameraLinkController::~PcoCameraLinkController()
{
    this->disconnect();
}

uint32_t PcoCameraLinkController::get_image_width(void)
{
    return image_width_;
}

uint32_t PcoCameraLinkController::get_image_height(void)
{
    return image_height_;
}

int PcoCameraLinkController::get_image_data_type(void)
{
    // TODO get this from common header?
    return 2;
}

std::size_t PcoCameraLinkController::get_image_size(void)
{
    std::size_t image_size = image_width_ * image_height_ * image_pixel_size_;
    return image_size;
}

bool PcoCameraLinkController::acquire_image(void* image_buffer)
{
    bool acquire_ok = true;
    DWORD pco_error;

        pco_error = grabber_->Acquire_Image(reinterpret_cast<WORD*>(image_buffer));

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

int PcoCameraLinkController::image_nr_from_timestamp(void *buf,int shift)
{
  unsigned short *b;
  int y;
  int image_nr=0;
  b=(unsigned short *)(buf);

  y=100*100*100;
  for(;y>0;y/=100)
  {
   *b>>=shift;
   image_nr+= (((*b&0x00F0)>>4)*10 + (*b&0x000F))*y;
   b++;
  }
  return image_nr;
}
