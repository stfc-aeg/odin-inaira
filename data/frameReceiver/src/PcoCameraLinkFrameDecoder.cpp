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

using namespace FrameReceiver;

PcoCameraLinkFrameDecoder::PcoCameraLinkFrameDecoder() :
    FrameDecoder()
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
  // Pass the configuration message to the base class decoder
  FrameDecoder::init(config_msg);

  LOG4CXX_DEBUG_LEVEL(2, logger_, "Got decoder config message: " << config_msg.encode());

  controller_.reset(new PcoCameraLinkController(logger_));

}

const size_t PcoCameraLinkFrameDecoder::get_frame_buffer_size(void) const
{
    return 1000;
}

const size_t PcoCameraLinkFrameDecoder::get_frame_header_size(void) const
{
  return 0;
}

void PcoCameraLinkFrameDecoder::monitor_buffers(void)
{

}

void PcoCameraLinkFrameDecoder::get_status(const std::string param_prefix,
    OdinData::IpcMessage& status_msg)
{

}