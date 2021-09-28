/*
 *
 * PcoCameraLinkFrameDecoder.cpp
 *
 * odin-data frame decoder plugin for image capture from PCO CameraLink systems
 *
 * Created on: 20 July 2021
 *     Author: Tim Nicholls, STFC Detector Systems Software Group
 *
 */

#include "PcoCameraLinkFrameDecoder.h"
#include "version.h"
#include "InairaDefinitions.h"

using namespace FrameReceiver;

PcoCameraLinkFrameDecoder::PcoCameraLinkFrameDecoder() :
    FrameDecoderCameraLink(),
    acquiring_(false),
    frames_acquired_(0)
{

    this->logger_ = Logger::getLogger("FR.PcoCLFrameDecoder");
    LOG4CXX_INFO(logger_, "PcoCameraLinkFrameDecoder version " << this->get_version_long() << " loaded");
}

// Destructor for PcoCameraLinkFrameDecoder
PcoCameraLinkFrameDecoder::~PcoCameraLinkFrameDecoder()
{
   LOG4CXX_DEBUG_LEVEL(2, logger_, "PcoCameraLinkFrameDecoder cleanup");
}

int PcoCameraLinkFrameDecoder::get_version_major()
{
  return ODIN_DATA_VERSION_MAJOR;
}

int PcoCameraLinkFrameDecoder::get_version_minor()
{
  return ODIN_DATA_VERSION_MINOR;
}

int PcoCameraLinkFrameDecoder::get_version_patch()
{
  return ODIN_DATA_VERSION_PATCH;
}

std::string PcoCameraLinkFrameDecoder::get_version_short()
{
  return ODIN_DATA_VERSION_STR_SHORT;
}

std::string PcoCameraLinkFrameDecoder::get_version_long()
{
  return ODIN_DATA_VERSION_STR;
}

void PcoCameraLinkFrameDecoder::init(LoggerPtr& logger, OdinData::IpcMessage& config_msg)
{
  LOG4CXX_DEBUG_LEVEL(2, logger_, "Got decoder config message: " << config_msg.encode());

  // Pass the configuration message to the base class decoder
  FrameDecoderCameraLink::init(config_msg);

  controller_.reset(new PcoCameraLinkController(this));

}

const size_t PcoCameraLinkFrameDecoder::get_frame_buffer_size(void) const
{
  size_t frame_buffer_size = get_frame_header_size() + controller_->get_image_size();
  LOG4CXX_DEBUG_LEVEL(2, logger_, "Calculated frame buffer size: " << frame_buffer_size);
  return frame_buffer_size;
}

const size_t PcoCameraLinkFrameDecoder::get_frame_header_size(void) const
{
  return sizeof(Inaira::FrameHeader);
}

void PcoCameraLinkFrameDecoder::monitor_buffers(void)
{

}

void PcoCameraLinkFrameDecoder::handle_ctrl_channel(void)
{
  std::string client_identity;
  std::string ctrl_req_encoded = ctrl_channel_.recv(&client_identity);

  IpcMessage ctrl_reply;
  IpcMessage::MsgVal ctrl_reply_val = IpcMessage::MsgValIllegal;

  bool request_ok = true;
  std::ostringstream error_ss;

  try
  {
    IpcMessage ctrl_req(ctrl_req_encoded.c_str(), false);
    IpcMessage::MsgType req_type = ctrl_req.get_msg_type();
    IpcMessage::MsgVal req_val = ctrl_req.get_msg_val();

    ctrl_reply.set_msg_id(ctrl_req.get_msg_id());
    ctrl_reply.set_msg_type(OdinData::IpcMessage::MsgTypeAck);

    switch (req_type)
    {
      case IpcMessage::MsgTypeCmd:

        switch (req_val)
        {
          case IpcMessage::MsgValCmdConfigure:
            LOG4CXX_DEBUG_LEVEL(3, logger_,
              "Got camera control configure request from client " << client_identity
                << " : " << ctrl_req_encoded
            );
            this->configure(ctrl_req, ctrl_reply);
            break;

          case IpcMessage::MsgValCmdStatus:
            LOG4CXX_DEBUG_LEVEL(3, logger_,
              "Got camera control status request from client " << client_identity
                << " : " << ctrl_req_encoded
            );
            this->get_status(std::string(""), ctrl_reply);
            break;

          default:
            request_ok = false;
            error_ss << "Illegal command request value: " << req_val;
            break;
        }
        break;

      default:
        request_ok = false;
        error_ss << "Illegal command request type: " << req_type;
        break;
    }

    ctrl_reply.set_msg_val(req_val);

  }
  catch (IpcMessageException& e)
  {
    request_ok = false;
    error_ss << e.what();
  }

  if (!request_ok) {
    LOG4CXX_ERROR(logger_, "Error handling camera control channel request from client "
                  << client_identity << ": " << error_ss.str());
    ctrl_reply.set_msg_type(IpcMessage::MsgTypeNack);
    ctrl_reply.set_param<std::string>("error", error_ss.str());
  }

  ctrl_channel_.send(ctrl_reply.encode(), 0, client_identity);

}

void PcoCameraLinkFrameDecoder::configure(
  OdinData::IpcMessage& config_msg, OdinData::IpcMessage& config_reply
)
{
  if (config_msg.has_param("command"))
  {
    std::string command = config_msg.get_param<std::string>("command");
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Config request has command: " << command);
    controller_->execute_command(command);
  }
}

void PcoCameraLinkFrameDecoder::get_status(const std::string param_prefix,
    OdinData::IpcMessage& status_msg)
{

  status_msg.set_param(param_prefix + "name", std::string("PcoCameraLinkFrameDecoder"));

  std::string camera_prefix = param_prefix + "camera/";
  status_msg.set_param(camera_prefix + "state", controller_->camera_state_name());

  std::string acq_prefix = param_prefix + "acquisition/";
  status_msg.set_param(acq_prefix + "acquiring", acquiring_);
  status_msg.set_param(acq_prefix + "frames_acquired", frames_acquired_);

}


void PcoCameraLinkFrameDecoder::run_camera_service(void)
{
  LOG4CXX_DEBUG_LEVEL(2, logger_, "PCO camera service thread entry");

  // Retrieve the image dimensions, data type and size from the controller as these values will not
  // from image to image
  uint32_t image_height = controller_->get_image_height();
  uint32_t image_width = controller_->get_image_width();
  std::size_t image_size = controller_->get_image_size();
  uint32_t image_data_type = controller_->get_image_data_type();

  unsigned int frame_count = 0;

  acquiring_ = false;
  frames_acquired_ = 0;

  while (run_thread_)
  {
    if (controller_->camera_running())
    {

      if (!acquiring_)
      {
        LOG4CXX_DEBUG_LEVEL(1, logger_, "Decoder now acquiring frames");
        acquiring_ = true;
        frames_acquired_ = 0;
      }

      int buffer_id;
      void* buffer_addr;
      if (this->get_empty_buffer(buffer_id, buffer_addr))
      {
        LOG4CXX_DEBUG_LEVEL(2, logger_, "Decoder got empty buffer id " << buffer_id
          << " at addr 0x" << std::hex << (unsigned long)buffer_addr << std::dec );

        void* image_buffer = reinterpret_cast<void*>(
          reinterpret_cast<uint8_t*>(buffer_addr) + this->get_frame_header_size()
        );

        if (controller_->acquire_image(image_buffer))
        {
          Inaira::FrameHeader* frame_hdr = reinterpret_cast<Inaira::FrameHeader*>(buffer_addr);
          frame_hdr->frame_number = frames_acquired_;
          frame_hdr->frame_width = image_width;
          frame_hdr->frame_height = image_height;
          frame_hdr->frame_data_type = image_data_type;
          frame_hdr->frame_size = image_size;

          this->notify_frame_ready(buffer_id, frames_acquired_);
          frames_acquired_++;
        }
      }
      else
      {
        LOG4CXX_WARN(logger_, "Failed to get empty buffer from queue");
      }
      usleep(1000000);
    }
    else
    {
      if (acquiring_)
      {
        LOG4CXX_DEBUG_LEVEL(1, logger_, "Decoder finished acquiring after "
          << frame_count << " frames");
        acquiring_ = false;
      }
    }
  }

  LOG4CXX_DEBUG_LEVEL(2, logger_, "PCO camera service thread exit");
}
