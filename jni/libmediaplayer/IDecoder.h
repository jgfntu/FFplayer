
#ifndef __IDECODER_H__
#define __IDECODER_H__
extern AVPacket BOS_PKT;
extern AVPacket EOS_PKT;

#include "Thread.h"

using namespace ffplayer;

namespace ffplayer
{
    class IDecoder
    {
        public:
            enum DECODER_STATUS
            {
                STATUS_IDLE, STATUS_INIT, STATUS_PREPARE, STATUS_STARTED, STATUS_STOPPED, STATUS_SEEKING
            };

            enum { RET_SCCESS, RET_ERROR };

            virtual void flush() = 0;

            virtual int stop() = 0;

            virtual int start() = 0;

            virtual int pause() = 0;

            virtual ~IDecoder()
            {
            }

            ;
    };
}
#endif //__IDECODER_H__


//~ Formatted by Jindent --- http://www.jindent.com
