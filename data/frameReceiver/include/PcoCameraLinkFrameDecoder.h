 /*
 * PcoCameraLinkFrameDecoder.h
 *
 *  Created on: 20 July 2021
 *      Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#ifndef INCLUDE_PCOCAMERALINKFRAMEDECODER_H_
#define INCLUDE_PCOCAMERALINKFRAMEDECODER_H_

#include <iostream>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include "FrameDecoderCameraLink.h"
#include "PcoCameraLinkController.h"

namespace FrameReceiver
{
  class PcoCameraLinkFrameDecoder : public FrameDecoderCameraLink
  {
  public:
    PcoCameraLinkFrameDecoder();
    ~PcoCameraLinkFrameDecoder();

    int get_version_major();
    int get_version_minor();
    int get_version_patch();
    std::string get_version_short();
    std::string get_version_long();

    void init(LoggerPtr& logger, OdinData::IpcMessage& config_msg);

    const size_t get_frame_buffer_size(void) const;
    const size_t get_frame_header_size(void) const;

    void monitor_buffers(void);

    void handle_ctrl_channel(void);
    void configure(OdinData::IpcMessage& config_msg, OdinData::IpcMessage& config_reply);
    void request_configuration(const std::string param_prefix, OdinData::IpcMessage& config_reply);
    void get_status(const std::string param_prefix, OdinData::IpcMessage& status_msg);

    const bool run_camera_service_thread(void) const { return run_thread_; }

  private:

    void run_camera_service(void);
    boost::scoped_ptr<PcoCameraLinkController> controller_;

  };

}

#endif // INCLUDE_PCOCAMERALINKFRAMEDECODER_H_
