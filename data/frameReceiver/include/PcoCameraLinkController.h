/*
 * PcoCameraLinkController.h
 *
 * Created on: 20 July 2021
 *     Author: Tim Nicholls, STFC Detector Systems Group
 */

#ifndef INCLUDE_PCOCAMERALINKCONTROLLER_H_
#define INCLUDE_PCOCAMERALINKCONTROLLER_H_

#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include <log4cxx/logger.h>
using namespace log4cxx;
using namespace log4cxx::helpers;
#include <DebugLevelLogger.h>
#include <IpcMessage.h>

#include "PcoCameraStateMachine.h"
#include "PcoCameraConfiguration.h"
#include "Cpco_com.h"
#include "Cpco_grab_clhs.h"

using namespace OdinData;

namespace FrameReceiver
{

  class PcoCameraLinkFrameDecoder; // forward declaration

  class PcoCameraLinkController
  {
  public:
    PcoCameraLinkController(PcoCameraLinkFrameDecoder* decoder);
    ~PcoCameraLinkController();

    uint32_t get_image_width(void);
    uint32_t get_image_height(void);
    int get_image_data_type(void);
    std::size_t get_image_size(void);

    void execute_command(std::string& command);

    bool acquire_image(void* image_buffer);

    void disconnect(void);
    void connect(void);
    void arm(void);
    void disarm(void);
    void start_recording(void);
    void stop_recording(void);

    std::string camera_state_name(void);
    bool camera_running(void) { return camera_running_; }

  private:

    int image_nr_from_timestamp(void *buf,int shift);

    LoggerPtr logger_;
    PcoCameraLinkFrameDecoder* decoder_;
    PcoCameraState camera_state_;
    PcoCameraConfiguration camera_config_;
    boost::shared_ptr<boost::thread> controller_thread_;
    std::string notify_endpoint_;

    bool run_thread_;

    boost::scoped_ptr<CPco_com> camera_;
    boost::scoped_ptr<CPco_grab_clhs> grabber_;

    bool camera_opened_;
    bool grabber_opened_;
    volatile bool camera_running_;

    int camera_num_;
    int grabber_timeout_ms_;

    uint32_t image_width_;
    uint32_t image_height_;
    uint32_t image_pixel_size_;

  };
}
#endif // INCLUDE_PCOCAMERALINKCONTROLLER_H_
