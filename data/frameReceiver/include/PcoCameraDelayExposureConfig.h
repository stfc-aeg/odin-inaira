/*!
 * PcoCameraDelayExposureConfig.h - PCO camera delay and exposure configuration container
 *
 * Created on: Oct 20, 2021
 *     Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#ifndef PCOCAMERADELAYEXPOSURECONFIG_H_
#define PCOCAMERADELAYEXPOSURECONFIG_H_

#include <map>

#include "Cpco_com.h"

namespace FrameReceiver
{
    //! PcoCameraDelayExposure - PCO camera delay and exposure configuration settings container
    class PcoCameraDelayExposure
    {
        public:

            //! Camera delay and exposure timebase values
            enum PcoCameraTimebase
            {
                TimebaseUnknown = -1, //!< Unknown timebase
                TimebaseNs = 0,       //!< Timebase in nanoseconds
                TimebaseUs = 1,       //!< Timebase in microseconds
                TimebaseMs = 2,       //!< Timebase in milliseconds
            };

            //! Internal mappings of timebases to time values and unit string names
            typedef std::map<PcoCameraTimebase, double> TimebaseValueMap;
            typedef std::map<PcoCameraTimebase, std::string> TimebaseNameMap;

            //! Default constructor
            PcoCameraDelayExposure();

            //! Constructor taking exposure time and frame rate as arguments
            PcoCameraDelayExposure(double exposure_time, double frame_rate);

            //! Returns the current exposure timebase unit as a string
            const std::string exposure_timebase_unit(void);

            //! Returns the current delay timebase unit as a string
            const std::string delay_timebase_unit(void);

            //! Returns the current exposure time in seconds as a double
            double exposure_time(void);

            //! Returns the current frame rate in Hertz as a double
            double frame_rate(void);

            //! Equality and inequality operators
            bool operator==(const PcoCameraDelayExposure& other);
            bool operator!=(const PcoCameraDelayExposure& other) { return !(*this == other); }

        private:

            //! Returns the timebase value in seconds for the specified timebase
            const double timebase_value(unsigned int timebase);

            //! Returns the timebase name as a string for the specified timebase
            const std::string timebase_name(unsigned int timebase);

            //! Selects the appropriate timebase value for a specified desired time value
            PcoCameraTimebase select_timebase(double time_value);

            DWORD exposure_time_;     //!< Exposure config setting in current timebase
            DWORD delay_time_;        //!< Delay config setting in current timebase
            WORD exposure_timebase_;  //!< Current exposure timebase config setting
            WORD delay_timebase_;     //!< Current delay timebase config setting

            //! Static declaration of the internal timebase value and name maps
            static TimebaseValueMap timebase_value_map_;
            static TimebaseNameMap timebase_name_map_;

            //! Allow the PcoCameraLinkController class direct access to config fields
            friend class PcoCameraLinkController;
    };
}

#endif //  PCOCAMERADELAYEXPOSURECONFIG_H_