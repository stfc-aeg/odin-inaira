/*
 * PcoCameraLinkFrameDecoder.h
 *
 *  Created on: 20 July 2021
 *      Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#ifndef INCLUDE_PCOCAMERALINKFRAMEDECODER_H_
#define INCLUDE_PCOCAMERALINKFRAMEDECODER_H_

#include "FrameDecoder.h"
#include <iostream>

namespace FrameReceiver
{
  class PcoCameraLinkFrameDecoder : public FrameDecoder
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
    void get_status(const std::string param_prefix, OdinData::IpcMessage& status_msg);
  };

}

#endif // INCLUDE_PCOCAMERALINKFRAMEDECODER_H_
