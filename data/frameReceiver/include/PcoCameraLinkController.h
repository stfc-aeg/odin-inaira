/*
 * PcoCameraLinkController.h
 *
 * Created on: 20 July 2021
 *     Author: Tim Nicholls, STFC Detector Systems Group
 */

#ifndef INCLUDE_PCOCAMERALINKCONTROLLER_H_
#define INCLUDE_PCOCAMERALINKCONTROLLER_H_

#include <boost/scoped_ptr.hpp>

#include <log4cxx/logger.h>
using namespace log4cxx;
using namespace log4cxx::helpers;
#include <DebugLevelLogger.h>

#include "Cpco_com.h"
#include "Cpco_grab_clhs.h"

class PcoCameraLinkController
{
public:
  PcoCameraLinkController(LoggerPtr& logger);
  ~PcoCameraLinkController();

private:

    void cleanup(void);
    int image_nr_from_timestamp(void *buf,int shift);

    LoggerPtr logger_;
    boost::scoped_ptr<CPco_com> camera_;
    boost::scoped_ptr<CPco_grab_clhs> grabber_;

    bool camera_opened_;
    bool grabber_opened_;
    bool camera_running_;

    int camera_num_;
    int grabber_timeout_ms_;

};

#endif // INCLUDE_PCOCAMERALINKCONTROLLER_H_
