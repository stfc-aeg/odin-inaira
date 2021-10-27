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

void PcoCameraLinkController::update_configuration(ParamContainer::Document& params)
{
    camera_config_.update(params);

    PcoCameraDelayExposure new_delay_exp(camera_config_.exposure_time_, camera_config_.frame_rate_);
    if (new_delay_exp != camera_delay_exp_)
    {
        LOG4CXX_DEBUG_LEVEL(2, logger_, "Updating camera delay and exposure settings");

        DWORD pco_error;
        bool updated = true;

        pco_error = camera_->PCO_SetTimebase(
            new_delay_exp.delay_timebase_, new_delay_exp.exposure_timebase_
        );
        if (pco_error != PCO_NOERROR)
        {
            LOG4CXX_ERROR(logger_, "Failed to set timebase with error code 0x"
                << std::hex << pco_error << std::dec);
                updated = false;
        }

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

void PcoCameraLinkController::get_configuration(ParamContainer::Document& params)
{
    camera_config_.encode(params, "camera");
}

void PcoCameraLinkController::get_status(ParamContainer::Document& params,
    const std::string param_prefix)
{
    camera_status_.camera_state_name_ = camera_state_.current_state_name();
    camera_status_.encode(params, param_prefix);
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

    camera_status_.camera_name_ = std::string(camera_infostr);
    camera_status_.camera_type_ = camera_type;
    camera_status_.camera_serial_ = camera_serial;

    LOG4CXX_INFO(logger_, "Connected to PCO camera with name: '" << camera_infostr
        << "' type: 0x" << std::hex << camera_type << std::dec
        << " serial number: " << camera_serial
    );

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

    camera_config_.exposure_time_ = camera_delay_exp_.exposure_time();
    camera_config_.frame_rate_ = camera_delay_exp_.frame_rate();

    LOG4CXX_INFO(logger_, "Camera reports exposure time: "
        << camera_delay_exp_.exposure_time_ << camera_delay_exp_.exposure_timebase_unit()
        << " delay time: " << camera_delay_exp_.delay_time_ << camera_delay_exp_.delay_timebase_unit()
        << " frame rate: " << camera_config_.frame_rate_ << "Hz"
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

void PcoCameraLinkController::run_camera_service(void)
{

  camera_status_.acquiring_ = false;
  camera_status_.frames_acquired_ = 0;

  int timeout = 0;
  DWORD pco_error;

  while (decoder_->run_camera_service_thread())
  {
    if (camera_running_)
    {

      if (!camera_status_.acquiring_)
      {

        grabber_->Get_Grabber_Timeout(&timeout);

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
        LOG4CXX_DEBUG_LEVEL(1, logger_, "Camera controller now acquiring " << ss.str() << " frames");
        camera_status_.acquiring_ = true;
        camera_status_.frames_acquired_ = 0;
      }

      int buffer_id;
      void* buffer_addr;
      if (decoder_->get_empty_buffer(buffer_id, buffer_addr))
      {
        LOG4CXX_DEBUG_LEVEL(2, logger_, "Decoder got empty buffer id " << buffer_id
          << " at addr 0x" << std::hex << (unsigned long)buffer_addr << std::dec );

        void* image_buffer = reinterpret_cast<void*>(
          reinterpret_cast<uint8_t*>(buffer_addr) + decoder_->get_frame_header_size()
        );

        if (this->acquire_image(image_buffer, timeout))
        {
          Inaira::FrameHeader* frame_hdr = reinterpret_cast<Inaira::FrameHeader*>(buffer_addr);
          frame_hdr->frame_number = camera_status_.frames_acquired_;
          frame_hdr->frame_width = image_width_;
          frame_hdr->frame_height = image_height_;
          frame_hdr->frame_data_type = image_data_type_;
          frame_hdr->frame_size = get_image_size();

          decoder_->notify_frame_ready(buffer_id, camera_status_.frames_acquired_);
          camera_status_.frames_acquired_++;
        }
      }
      else
      {
        LOG4CXX_WARN(logger_, "Failed to get empty buffer from queue");
      }
      if (camera_config_.num_frames_ && (camera_status_.frames_acquired_ >= camera_config_.num_frames_))
      {
        pco_error = grabber_->Stop_Acquire();
        if (pco_error != PCO_NOERROR)
        {
            LOG4CXX_ERROR(logger_, "Failed to stop frame grabber acquisition with error code 0x"
                << std::hex << pco_error << std::dec);
        }
        LOG4CXX_INFO(logger_,
            "Camera controller completed acquisition of " << camera_status_.frames_acquired_ << " frames"
        );
        camera_status_.acquiring_ = false;
        camera_state_.execute_command(PcoCameraState::CommandType::CommandStopRecording);
      }
      else
      {
        //usleep(1000000); // TODO remove this hardcoded delay
      }
    }
    else
    {
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
    return image_data_type_;
}

std::size_t PcoCameraLinkController::get_image_size(void)
{
    std::size_t image_size = image_width_ * image_height_ * image_pixel_size_;
    return image_size;
}

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
