/*
 * PcoCameraLinkFrameDecoderLib.cpp
 *
 *  Created on: 20 July 2021
 *      Author: Tim Nicholls, STFC Detector Systems Software Group
 */

#include "PcoCameraLinkFrameDecoder.h"
#include "ClassLoader.h"

namespace FrameReceiver
{
  /**
   * Registration of this decoder through the ClassLoader.  This macro
   * registers the class without needing to worry about name mangling
   */
  REGISTER(FrameDecoder, PcoCameraLinkFrameDecoder, "PcoCameraLinkFrameDecoder");

}
// namespace FrameReceiver