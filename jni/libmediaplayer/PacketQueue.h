
#ifndef __PACKETQUEUE_H
#define __PACKETQUEUE_H

#include <pthread.h>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}    // end of extern C

namespace ffplayer
{
    class PacketQueue
    {
        public:
            class Observer
            {
                public:
                    enum { DATA_CONSUMED, DATA_INCREASED };

                    virtual void onDataChanged(int evt) = 0;
            };


            /*
             * packetqueue operation result/status code
             */
            enum
            {
                /* operation success */
                PQ_OP_SUCCESS,

                /* memory related error */
                PQ_OP_MEM_ERROR,

                /* input parameter invalid */
                PQ_OP_IN_PARA_INVALID,

                /* abort request */
                PQ_OP_ABORT_REQ,

                /* no more pkt */
                PQ_OP_NO_MORE_PKT,
            };

            /*
             * constructor and destructor
             */
            PacketQueue(PacketQueue::Observer * obs);

            ~PacketQueue();

            /*
             * clean all the avpacket in this queue and release related resources
             */
            void flush();

            /*
             * put a packet to the queue
             */
            int put(AVPacket * pkt);

            /* return < 0 if aborted, 0 if no packet and > 0 if packet. */
            int get(AVPacket * pkt,
                    bool       block);

            int size();

            void abort();

        private:
            AVPacketList *          mFirst;
            AVPacketList *          mLast;
            int                     mNbPackets;
            bool                    mAbortRequest;
            pthread_mutex_t         mLock;
            pthread_cond_t          mCondition;
            PacketQueue::Observer * mObserver;
    };
}
#endif // __PACKETQUEUE_H


//~ Formatted by Jindent --- http://www.jindent.com
