#ifndef INCLUDE_INAIRADEFINITIONS_H_
#define INCLUDE_INAIRADEFINITIONS_H_

namespace Inaira
{
    typedef struct
    {
        uint32_t frame_number;
        // uint32_t frame_state;
        // uint64_t frame_start_time_secs;
        // uint64_t frame_start_nsecs;
        uint32_t frame_width;
        uint32_t frame_height;
        uint32_t frame_data_type;
        uint32_t frame_size;
    } FrameHeader;
}


#endif /*INCLUDE_INAIRADEFINITIONS_H_*/
