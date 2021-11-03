 /*
 * PcoCameraLinkFrameDecoder.h - frame decoder implementation for the PCO camera system
 *
 *  Created on: 20 July 2021
 *      Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#ifndef INCLUDE_PCOCAMERALINKFRAMEDECODER_H_
#define INCLUDE_PCOCAMERALINKFRAMEDECODER_H_

#include <iostream>
#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include "FrameDecoderCameraLink.h"
#include "PcoCameraLinkController.h"

namespace FrameReceiver
{

  const std::string CAMERA_CONFIG_PATH = "camera";
  const std::string CAMERA_COMMAND_PATH = "command";

  class PcoCameraLinkFrameDecoder : public FrameDecoderCameraLink
  {
  public:

    //! Constructor for the PcoCameraLinkFrameDecoder class
    PcoCameraLinkFrameDecoder();

    //! Destructor for the PcoCameraLinkFrameDecoder class
    ~PcoCameraLinkFrameDecoder();

    //! Returns the decoder version number major value
    int get_version_major();

    //! Returns the decoder version number minor value
    int get_version_minor();

    //! Returns the decoder version number patch value
    int get_version_patch();

    //! Returns the decoder version short string
    std::string get_version_short();

    //! Returns the decoder version long string
    std::string get_version_long();

    //! Initialises the decoder from a configuration message provided by the frame receiver
    void init(LoggerPtr& logger, OdinData::IpcMessage& config_msg);

    //! Returns the frame buffer size required for image acquisition
    const size_t get_frame_buffer_size(void) const;

    //! Returns the frame header size defined for this decoder
    const size_t get_frame_header_size(void) const;

    //! Monitors the state of allocated buffers in the decoder
    void monitor_buffers(void);

    //! Handles camera control channel messages
    void handle_ctrl_channel(void);

    //! Configures the decoder and camera based on the content of a configuration message
    void configure(OdinData::IpcMessage& config_msg, OdinData::IpcMessage& config_reply);

    //! Returns the current camera configuration parameters in an IPC message
    void request_configuration(const std::string param_prefix, OdinData::IpcMessage& config_reply);

    //! Returns the current status of the camera in an IPC message
    void get_status(const std::string param_prefix, OdinData::IpcMessage& status_reply);

    //! Indicates if the camera control service thread is currently running
    const bool run_camera_service_thread(void) const { return run_thread_; }

  private:

    //! Runs the camera control service
    void run_camera_service(void);

    //! Scoped pointer to the current PCO camera controller instance
    boost::scoped_ptr<PcoCameraLinkController> controller_;

  };

}

#endif // INCLUDE_PCOCAMERALINKFRAMEDECODER_H_
