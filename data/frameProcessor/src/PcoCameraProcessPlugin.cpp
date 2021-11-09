#include <DebugLevelLogger.h>

#include "PcoCameraProcessPlugin.h"
#include "version.h"

namespace FrameProcessor
{

    PcoCameraProcessPlugin::PcoCameraProcessPlugin()
    {
        // Set up logging
        logger_ = Logger::getLogger("FP.PcoCameraProcessPlugin");
        LOG4CXX_INFO(logger_, "PcoCameraProcessPlugin version " <<
            this->get_version_long() << " loaded.");
    }

    PcoCameraProcessPlugin::~PcoCameraProcessPlugin()
    {
        LOG4CXX_TRACE(logger_, "PcoCameraProcessPlugin destructor.");
    }

    void PcoCameraProcessPlugin::configure(
        OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
    {
        // TODO add config here
    }

    void PcoCameraProcessPlugin::requestConfiguration(OdinData::IpcMessage& reply)
    {
        //return the config of the plugin

    }

    void PcoCameraProcessPlugin::status(OdinData::IpcMessage& status)
    {
        //return the status of the plugin
        LOG4CXX_DEBUG(logger_, "Status requested for PcoCameraProcessPlugin");

    }

    bool PcoCameraProcessPlugin::reset_statistics(void)
    {
        return true;
    }

    void PcoCameraProcessPlugin::process_frame(boost::shared_ptr<Frame> frame)
    {
        Inaira::FrameHeader* hdr_ptr = static_cast<Inaira::FrameHeader*>(frame->get_data_ptr());

        LOG4CXX_DEBUG_LEVEL(1, logger_,
            "process_frame got frame number " << hdr_ptr->frame_number
            << " width " << hdr_ptr->frame_width
            << " height " << hdr_ptr->frame_height
            << " type " << hdr_ptr->frame_data_type
            << " size " << hdr_ptr->frame_size
        );

        FrameMetaData metadata;

        metadata.set_dataset_name("pco");
        metadata.set_data_type((DataType)hdr_ptr->frame_data_type);
        metadata.set_frame_number(hdr_ptr->frame_number);
        metadata.set_compression_type(no_compression);
        dimensions_t dims(2);
        dims[0] = hdr_ptr->frame_height;
        dims[1] = hdr_ptr->frame_width;
        metadata.set_dimensions(dims);

        frame->set_meta_data(metadata);
        frame->set_image_offset(sizeof(Inaira::FrameHeader));
        frame->set_image_size(hdr_ptr->frame_size);

        this->push(frame);
    }
}