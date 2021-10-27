/*!
 * PcoCameraDelayExposureConfig.cpp - PCO camera delay and exposure configuration container
 *
 * This class implements a container for the PCO camera delay and exposure configuration settings,
 * allowing them to easily be calculated and related to exposure time and frame rate. The camera
 * expresses these in terms of a time value and timebase unit for each of exposure and delay
 * parameters.
 *
 * Created on: Oct 20, 2021
 *     Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#include "PcoCameraDelayExposureConfig.h"

namespace FrameReceiver
{

    //! Default constructor for PcoCameraDelayExposure
    //!
    //! This default constructor simply initialises all attributes of the class to zero
    //! initial values
    PcoCameraDelayExposure::PcoCameraDelayExposure() :
        exposure_time_(0),
        delay_time_(0),
        exposure_timebase_(0),
        delay_timebase_(0)
    { }

    //! Constructor taking exposure time and frame rate values as arguments.
    //!
    //! The constructor initialises the PcoCameraDelayExposure based on the specified exposure
    //! time and frame rate arguments, calculating the appropriate exposure and delay time and
    //! timebase setting values.
    //!
    //! \param exposure_time - double precision exposure time in seconds
    //! \param frame_rate    - double precision frame rate in Hertz

    PcoCameraDelayExposure::PcoCameraDelayExposure(double exposure_time, double frame_rate)
    {
        // Calculate the exposure timebase and time from the specified value
        exposure_timebase_ = select_timebase(exposure_time);
        exposure_time_ = static_cast<DWORD>(exposure_time / timebase_value(exposure_timebase_));

        // Calculate the frame period and delay time based on the specified rate and exposure time
        double frame_period = 1.0 / frame_rate;
        double delay_time = frame_period - exposure_time;

        // Calculate the approproate delay timebase and time value
        delay_timebase_ = select_timebase(delay_time);
        delay_time_ = static_cast<DWORD>(delay_time / timebase_value(delay_timebase_));
    }

    //! Returns the exposure timebase unit as a string
    //!
    //! This method returns the exposure timebase unit as a string.
    //!
    //! \return timebase unit name as a string

    const std::string PcoCameraDelayExposure::exposure_timebase_unit(void) {
        return timebase_name(exposure_timebase_);
    }

    //! Returns the delay timebase unit as a string
    //!
    //! This method returns the delay timebase unit as a string.
    //!
    //! \return timebase unit name as a string

    const std::string PcoCameraDelayExposure::delay_timebase_unit(void) {
        return timebase_name(delay_timebase_);
    }

    //! Returns the exposure time in seconds as a double
    //!
    //! This method calculated and returns the exposure time in seconds as a double.
    //!
    //! \return exposure time in seconds as a double

    double PcoCameraDelayExposure::exposure_time(void) {
        return (exposure_time_ * timebase_value(exposure_timebase_));
    }

    //! Returns the frame rate in Hertz as a double
    //!
    //! This method calculated and returns the frame rate in Hertz as a double.
    //!
    //! \return frame rate in Hertz as a double

    double PcoCameraDelayExposure::frame_rate(void) {
        return 1.0 / (exposure_time() + (delay_time_ * timebase_value(delay_timebase_)));
    }

    //! Equality operator
    //!
    //! The method implements the equality operator for the PcoCameraDelayExposure class,
    //! comparing each of the exposure and delay time and timebase values.
    //!
    //! \param other - reference to other PcoCameraDelayExposure object being compared
    //! \return bool equality, true when all fields match

    bool PcoCameraDelayExposure::operator==(const PcoCameraDelayExposure& other
    )
    {
        return (
            (exposure_time_ == other.exposure_time_) &&
            (delay_time_ == other.delay_time_) &&
            (exposure_timebase_== other.exposure_timebase_) &&
            (delay_timebase_ == other.delay_timebase_)
        );
    }

    // Private methods

    //! Returns the timebase value in seconds for the specified timebase
    //!
    //! This private method returns the timebase value in seconds for a given timebase
    //! value.
    //!
    //! \param timebase - timebase to determine value for
    //! \return timebase value in seconds as a double

    const double PcoCameraDelayExposure::timebase_value(unsigned int timebase)
    {
        double value = 0.0;

        if (timebase_value_map_.size() == 0)
        {
            timebase_value_map_[TimebaseNs] = 1.0E-9;
            timebase_value_map_[TimebaseUs] = 1.0E-6;
            timebase_value_map_[TimebaseMs] = 1.0E-3;
        }

        if (timebase_value_map_.count((PcoCameraTimebase)timebase))
        {
            value = timebase_value_map_[(PcoCameraTimebase)timebase];
        }
        return value;
    }

    //! Returns the timebase name as a string for the specified timebase
    //!
    //! This private method returns the name of a the specified timebase as a string
    //!
    //! \param timebase - timebase setting
    //! \return name of the timebase setting as a string

    const std::string PcoCameraDelayExposure::timebase_name(unsigned int timebase)
    {
        std::string name = "??";

        if (timebase_name_map_.size() == 0)
        {
            timebase_name_map_[TimebaseNs] = "ns";
            timebase_name_map_[TimebaseUs] = "us";
            timebase_name_map_[TimebaseMs] = "ms";
        }

        if (timebase_name_map_.count((PcoCameraTimebase)timebase))
        {
            name = timebase_name_map_[(PcoCameraTimebase)timebase];
        }
        return name;
    }

    //! Selects the appropriate timebase value for a specified desired time value
    //!
    //! This private method selects the appropriate timebase for the specified time value (i.e. for
    //! either exposure or delay settings.)
    //!
    //! \param time_value time value in seconds
    //! \return appropriate timebase setting or unknown if no legal setting possible

    PcoCameraDelayExposure::PcoCameraTimebase PcoCameraDelayExposure::select_timebase(
        double time_value
    )
    {
        PcoCameraTimebase timebase = PcoCameraTimebase::TimebaseUnknown;

        // Select appropriate exposure timebase - TODO handle max/min exposure limits
        if (time_value < timebase_value(PcoCameraTimebase::TimebaseUs))
        {
            timebase = PcoCameraTimebase::TimebaseNs;
        }
        else if (time_value < timebase_value(PcoCameraTimebase::TimebaseMs))
        {
            timebase = PcoCameraTimebase::TimebaseUs;
        }
        else
        {
            timebase = PcoCameraTimebase::TimebaseMs;
        }

        return timebase;
    }

    // Static declaration of timebase value and name maps
    PcoCameraDelayExposure::TimebaseValueMap PcoCameraDelayExposure::timebase_value_map_;
    PcoCameraDelayExposure::TimebaseNameMap PcoCameraDelayExposure::timebase_name_map_;
}