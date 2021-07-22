#include "PcoCameraLinkController.h"
#include "file12.h"

PcoCameraLinkController::PcoCameraLinkController(LoggerPtr& logger) :
    logger_(logger),
    camera_opened_(false),
    grabber_opened_(false),
    camera_running_(false),
    camera_num_(0),
    grabber_timeout_ms_(10000)
{
    DWORD pco_error;

    WORD camera_type;
    DWORD camera_serial;
    SC2_Camera_Description_Response camera_description;
    char camera_infostr[100];
    DWORD image_width,image_height, exp_time, delay_time;
    WORD* image_buffer;

    LOG4CXX_INFO(logger_, "PcoCameraLinkController: initialising camera system");

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Creating PCO camera object")
    camera_.reset(new CPco_com_clhs());
    if (camera_ == NULL)
    {
        LOG4CXX_ERROR(logger_, "Failed to create PCO camera object");
        this->cleanup();
        return;

    }

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Opening PCO camera " << camera_num_);
    pco_error = camera_->Open_Cam(camera_num_);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to open PCO camera with error code 0x"
            << std::hex << pco_error << std::dec);
        this->cleanup();
        return;
    }
    else
    {
        camera_opened_ = true;
    }

    pco_error = camera_->PCO_GetCameraType(&camera_type, &camera_serial);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger, "Failed to get camera type with error code 0x"
            << std::hex << pco_error << std::dec);
        this->cleanup();
        return;
    }
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Camera type " << camera_type << " serial " << camera_serial);

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Creating PCO grabber object")
    grabber_.reset(new CPco_grab_clhs((CPco_com_clhs*)(camera_.get())));
    if (grabber_ == NULL)
    {
        LOG4CXX_ERROR(logger_, "Failed to create PCO frame grabber object");
        this->cleanup();
        return;
    }

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Opening PCO grabber " << camera_num_);
    pco_error = grabber_->Open_Grabber(camera_num_);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to open PCO grabber with error code 0x"
            << std::hex << pco_error << std::dec);
        this->cleanup();
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
        this->cleanup();
        return;
    }

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Getting camera info");
    pco_error = camera_->PCO_GetCameraDescriptor(&camera_description);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to camera descriptor with error code 0x"
            << std::hex << pco_error << std::dec);
        this->cleanup();
        return;
    }
    pco_error = camera_->PCO_GetInfo(1, camera_infostr, sizeof(camera_infostr));
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to camera info with error code 0x"
            << std::hex << pco_error << std::dec);
        this->cleanup();
        return;
    }

     LOG4CXX_INFO(logger_, "Connected to PCO camera with name: '" << camera_infostr
        << "' type: 0x" << std::hex << camera_type << std::dec
        << " serial number: " << camera_serial
    )

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Arming camera");
    pco_error = camera_->PCO_ArmCamera();
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to arm camera with error code 0x"
            << std::hex << pco_error << std::dec);
        this->cleanup();
        return;
    }

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Post-arming grabber");
    pco_error = grabber_->PostArm();
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to post-arm grabber with error code 0x"
            << std::hex << pco_error << std::dec);
        this->cleanup();
        return;
    }

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Setting camera recording state to RUN");
    pco_error = camera_->PCO_SetRecordingState(1);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to set camera recoding state to RUN with error code 0x"
            << std::hex << pco_error << std::dec);
        this->cleanup();
        return;
    }
    camera_running_ = true;

    pco_error = grabber_->Get_actual_size(&image_width, &image_height, NULL);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to get actual size from grabber with error code 0x"
            << std::hex << pco_error << std::dec);
        this->cleanup();
        return;
    }
    LOG4CXX_INFO(logger_, "Grabber reports actual size: width: "
        << image_width << " height: " << image_height);

    pco_error = camera_->PCO_GetDelayExposure(&delay_time, &exp_time);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to get delay and exposure times with error code 0x"
             << std::hex << pco_error << std::dec);
        this->cleanup();
        return;
    }
    LOG4CXX_INFO(logger_, "Camera reports exposure time: "
        << exp_time << "ms delay time: " << delay_time << "ms");

    image_buffer = (WORD *)malloc(image_width * image_height * sizeof(WORD));
    if (image_buffer == NULL)
    {
        LOG4CXX_ERROR(logger_, "Failed to allocate image buffer");
        this->cleanup();
        return;
    }

    pco_error = grabber_->Acquire_Image(image_buffer);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to acquire an image with error code 0x"
             << std::hex << pco_error << std::dec);
        this->cleanup();
        return;
    }

    int image_num = this->image_nr_from_timestamp(image_buffer, 0);
    LOG4CXX_INFO(logger_, "Image acquisition completed OK with image number: " << image_num);

    char* filename = "inaira.tif";
    store_tif(filename, image_width, image_height, 0, image_buffer);

    pco_error = camera_->PCO_SetRecordingState(0);
    if (pco_error != PCO_NOERROR)
    {
        LOG4CXX_ERROR(logger_, "Failed to set recording state to stopped with error code 0x"
             << std::hex << pco_error << std::dec);
        this->cleanup();
        return;
    }
    camera_running_ = false;
}

PcoCameraLinkController::~PcoCameraLinkController()
{
    this->cleanup();
}

void PcoCameraLinkController::cleanup(void)
{
    LOG4CXX_INFO(logger_, "PcoCameraLinkController cleanup");

    if (camera_ && camera_running_) {
        LOG4CXX_DEBUG_LEVEL(2, logger_, "Cleanup: setting camera recording state to stop");
        camera_->PCO_SetRecordingState(0);
    }
    if (grabber_ && grabber_opened_) {
        LOG4CXX_DEBUG_LEVEL(2, logger_, "Cleanup: closing PCO grabber");
        grabber_->Close_Grabber();
        grabber_opened_ = false;
    }

    if (camera_ && camera_opened_) {
        LOG4CXX_DEBUG_LEVEL(2, logger_, "Cleanup: closing PCO camera");
        camera_->Close_Cam();
        camera_opened_ = false;
    }
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