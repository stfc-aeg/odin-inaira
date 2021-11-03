/*
 * PcoCameraLinkFrameDecoder.cpp - frame decoder implementation for the PCO camera system
 *
 * This class implements the odin-data frame decoder plugin for image capture from PCO CameraLink
 * systems. The decoder provides the standard interface between the frame receiver infrastructure
 * and the PCO camera controller instance, which controls image acquisition.
 *
 * Created on: 20 July 2021
 *     Author: Tim Nicholls, STFC Detector Systems Software Group
 *
 */

#include "PcoCameraLinkFrameDecoder.h"
#include "version.h"
#include "InairaDefinitions.h"

using namespace FrameReceiver;

//! Constructor for the PcoCameraLinkFrameDecoder class.
//!
//! This constructor performs the basic initialisation of the decoder instance. The base class
//! constructor is called and the logger instance initialised. The detailed initialisation of the
//! decoder and the PCO camera controller is done later during a call to the init() method.

PcoCameraLinkFrameDecoder::PcoCameraLinkFrameDecoder() :
    FrameDecoderCameraLink()
{

    this->logger_ = Logger::getLogger("FR.PcoCLFrameDecoder");
    LOG4CXX_INFO(logger_, "PcoCameraLinkFrameDecoder version " << this->get_version_long() << " loaded");
}

//! Destructor for PcoCameraLinkFrameDecoder class.
PcoCameraLinkFrameDecoder::~PcoCameraLinkFrameDecoder()
{
   LOG4CXX_DEBUG_LEVEL(2, logger_, "PcoCameraLinkFrameDecoder cleanup");
}

//! Returns the decoder version number major value.
//!
//! This method returns the major part of the decoder version number, which is derived from the
//! most recent git tag and commit history.
//!
//! \return major version number as an integer

int PcoCameraLinkFrameDecoder::get_version_major()
{
  return ODIN_DATA_VERSION_MAJOR;
}

//! Returns the decoder version number minor value.
//!
//! This method returns the minor part of the decoder version number, which is derived from the
//! most recent git tag and commit history.
//!
//! \return minor version number as an integer

int PcoCameraLinkFrameDecoder::get_version_minor()
{
  return ODIN_DATA_VERSION_MINOR;
}

//! Returns the decoder version number patch value.
//!
//! This method returns the patch part of the decoder version number, which is derived from the
//! most recent git tag and commit history.
//!
//! \return patch version number as an integer

int PcoCameraLinkFrameDecoder::get_version_patch()
{
  return ODIN_DATA_VERSION_PATCH;
}

//! Returns the decoder version short string.
//!
//! This method returns the short version of the formatted version number as a string. This is
//! derived from the most recent git tag and version history.
//!
//! \return decoder short version as a string

std::string PcoCameraLinkFrameDecoder::get_version_short()
{
  return ODIN_DATA_VERSION_STR_SHORT;
}

//! Returns the decoder version long string.
//!
//! This method returns the long version of the formatted version number as a string. This is
//! derived from the most recent git tag and version history
//!
//! \return decoder long version as a string

std::string PcoCameraLinkFrameDecoder::get_version_long()
{
  return ODIN_DATA_VERSION_STR;
}

//! Initialises the decoder from a configuration message provided by the frame receiver.
//!
//! This method initialises the decoder from the configuration message received as an argument. The
//! base class init() method is called and a new PcoCameraLinkController instance created to control
//! the camera.
//!
//! \param logger - deprecated argument retained for compatiblity
//! \param config_msg - IPC message containing decoder configuration parameters

void PcoCameraLinkFrameDecoder::init(LoggerPtr& logger, OdinData::IpcMessage& config_msg)
{
  LOG4CXX_DEBUG_LEVEL(2, logger_, "Got decoder config message: " << config_msg.encode());

  // Pass the configuration message to the base class decoder
  FrameDecoderCameraLink::init(config_msg);

  // Instantiate a new camera controller
  controller_.reset(new PcoCameraLinkController(this));

}

//! Returns the frame buffer size required for image acquisition.
//!
//! This method calcualtes and returns the frame buffer size required for image acqusition. This is
//! used by the frame reeiver controller during initialisation to configure the frame shared memory
//! buffer. The controller determines the raw image size based on the configuration of the camera.
//!
//! \return frame buffer size in bytes

const size_t PcoCameraLinkFrameDecoder::get_frame_buffer_size(void) const
{
  size_t frame_buffer_size = get_frame_header_size() + controller_->get_image_size();
  LOG4CXX_DEBUG_LEVEL(2, logger_, "Calculated frame buffer size: " << frame_buffer_size);
  return frame_buffer_size;
}

//! Returns the frame header size defined for this decoder.
//!
//! This method returns the size of the frame header used during image acqusition by this decoder.
//!
//! \return frame header size in bytes

const size_t PcoCameraLinkFrameDecoder::get_frame_header_size(void) const
{
  return sizeof(Inaira::FrameHeader);
}

//! Monitors the state of allocated buffers in the decoder.
//!
//! This method is intended to monitor the state of currently allocated buffers in the decoder and
//! release any timed-out buffers. In practice this is not currently implemented in this decoder
//! since the success of image acquisition is known immediately, rather than having to handle the
//! situation where packet loss occurs.

void PcoCameraLinkFrameDecoder::monitor_buffers(void)
{
  // Currently does nothing
}

//! Handles camera control channel messages.
//!
//! This method handles messages received on the camera control channel, decoding the incoming
//! IpcMessages and dispatching operations to appropriate method as necessary.

void PcoCameraLinkFrameDecoder::handle_ctrl_channel(void)
{

  // Receive the control channel request and store the client identity so that the response
  // can be routed back correctly.
  std::string client_identity;
  std::string ctrl_req_encoded = ctrl_channel_.recv(&client_identity);

  // Create a reply message
  IpcMessage ctrl_reply;
  IpcMessage::MsgVal ctrl_reply_val = IpcMessage::MsgValIllegal;

  bool request_ok = true;
  std::ostringstream error_ss;

  try
  {

    // Attempt to decode the incoming message and get the request type and value
    IpcMessage ctrl_req(ctrl_req_encoded.c_str(), false);
    IpcMessage::MsgType req_type = ctrl_req.get_msg_type();
    IpcMessage::MsgVal req_val = ctrl_req.get_msg_val();

    // Pre-populate the appropriate fields in the response
    ctrl_reply.set_msg_id(ctrl_req.get_msg_id());
    ctrl_reply.set_msg_type(OdinData::IpcMessage::MsgTypeAck);
    ctrl_reply.set_msg_val(req_val);

    // Handle the request according to its type
    switch (req_type)
    {
      // Handle command reqests
      case IpcMessage::MsgTypeCmd:

        // Handle command requests according to their value
        switch (req_val)
        {
          // Handle a configuration command
          case IpcMessage::MsgValCmdConfigure:
            LOG4CXX_DEBUG_LEVEL(3, logger_,
              "Got camera control configure request from client " << client_identity
                << " : " << ctrl_req_encoded
            );
            this->configure(ctrl_req, ctrl_reply);
            break;

          // Handle a configuration request command
          case IpcMessage::MsgValCmdRequestConfiguration:
            LOG4CXX_DEBUG_LEVEL(3, logger_,
              "Got camera control read configuration request from client " << client_identity
              << " : " << ctrl_req_encoded
            );
            this->request_configuration(std::string(""), ctrl_reply);
            break;

          // Handle a status request command
          case IpcMessage::MsgValCmdStatus:
            LOG4CXX_DEBUG_LEVEL(3, logger_,
              "Got camera control status request from client " << client_identity
                << " : " << ctrl_req_encoded
            );
            this->get_status(std::string(""), ctrl_reply);
            break;

          // Handle unsupported request values by setting the status and error message
          default:
            request_ok = false;
            error_ss << "Illegal command request value: " << req_val;
            break;
        }
        break;

      // Handle unsupported request types by setting the status and error message
      default:
        request_ok = false;
        error_ss << "Illegal command request type: " << req_type;
        break;
    }
  }

  // Handle exceptions thrown during message decoding, setting the status and error message
  // accordingly
  catch (IpcMessageException& e)
  {
    request_ok = false;
    error_ss << e.what();
  }

  // If the request could not be decoded or handled, set the response type to NACK and populate
  // the error parameter with the error string
  if (!request_ok) {
    LOG4CXX_ERROR(logger_, "Error handling camera control channel request from client "
                  << client_identity << ": " << error_ss.str());
    ctrl_reply.set_msg_type(IpcMessage::MsgTypeNack);
    ctrl_reply.set_param<std::string>("error", error_ss.str());
  }

  // Send the encoded response back to the client
  ctrl_channel_.send(ctrl_reply.encode(), 0, client_identity);

}

//! Configures the decoder and camera based on the content of a configuration message.
//!
//! This method is called in response to receipt of a configuration command message on the
//! control channel. If the message parameter payload includes camera parameters, these are
//! passed to the camera controller for handling. If the payload contains a command parameter,
//! this is passed to the controller to be executed.
//!
//! TODO - handle failure cases here so that response to client is updated
//!
//! \param config_msg - incoming configuration IPC message
//! \param config_reply - reply to configuration message, currently unused

void PcoCameraLinkFrameDecoder::configure(
  OdinData::IpcMessage& config_msg, OdinData::IpcMessage& config_reply
)
{
  // If the configuration message has camera parameters, copy those into a parameter document
  // and tell the controller to update its configuration
  if (config_msg.has_param(CAMERA_CONFIG_PATH))
  {
    ParamContainer::Document config_params;
    config_msg.encode_params(config_params, CAMERA_CONFIG_PATH);
    controller_->update_configuration(config_params);
  }

  // If the configuration message has a command parameter, extract the command value and pass
  // to the controller
  if (config_msg.has_param(CAMERA_COMMAND_PATH))
  {
    std::string command = config_msg.get_param<std::string>(CAMERA_COMMAND_PATH);
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Config request has command: " << command);
    controller_->execute_command(command);
  }
}

//! Returns the current camera configuration parameters in an IPC message.
//!
//! This method is called in response to a configuration request message received on the
//! control channel. The reply is populated with the current configuration parameters from
//! the base class and from the camera controller.
//!
//! \param param_prefix - parameter prefix for reply as a string
//! \param config_reply - reference to config reply message to populate
void PcoCameraLinkFrameDecoder::request_configuration(
  const std::string param_prefix, OdinData::IpcMessage& config_reply
)
{

  // Call the base class method to populate parameters
  FrameDecoderCameraLink::request_configuration(param_prefix, config_reply);

  // Create a new param document and pass to the controller to populate with the appropriate
  // parameter prefix
  ParamContainer::Document camera_config;
  std::string camera_config_prefix = param_prefix + CAMERA_CONFIG_PATH;
  controller_->get_configuration(camera_config, camera_config_prefix);

  // Update the reply message parameters with the config document
  config_reply.update(camera_config);
}

//! Returns the current status of the camera in an IPC message.
//!
//! This method is called in response to a status request received on the control channel. The
//! reply is populated with current status parameters retrieved from the controller.
//!
//! \param param_prefix - parameter prefix for reply as a string
//! \param status_reply - reference to status reply message to populate

void PcoCameraLinkFrameDecoder::get_status(
  const std::string param_prefix, OdinData::IpcMessage& status_reply
)
{

  // Insert the decoder name into the reply
  status_reply.set_param(param_prefix + "name", std::string("PcoCameraLinkFrameDecoder"));

  // Create a new parame document and pass to the controller to populate with the appropriate
  // parameter prefix
  ParamContainer::Document camera_status;
  controller_->get_status(camera_status, param_prefix);

  // Update the reply message parameters with the status document
  status_reply.update(camera_status);
}

// Private methods

//! Runs the camera control service.
//!
//! This method runs the camera control service by calling the equivalent method in the controller
//! which is responsible for controlling the camera and acquiring images

void PcoCameraLinkFrameDecoder::run_camera_service(void)
{
  LOG4CXX_DEBUG_LEVEL(2, logger_, "PCO camera service thread entry");
  controller_->run_camera_service();
  LOG4CXX_DEBUG_LEVEL(2, logger_, "PCO camera service thread exit");
}
