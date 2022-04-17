#include "AVDecodeQueueBox.hpp"

#include "EyerDecodeQueue/EyerDecodeQueueHeader.hpp"

namespace Eyer
{
    AVDecodeQueueBox::AVDecodeQueueBox()
    {

    }

    AVDecodeQueueBox::~AVDecodeQueueBox()
    {
        ClearAllStream();
        StopAllDecoder();
    }

    int AVDecodeQueueBox::AddStream(const EyerAVStream & stream)
    {
        streamList.push_back(stream);
        return 0;
    }

    int AVDecodeQueueBox::ClearAllStream()
    {
        streamList.clear();
        return 0;
    }

    int AVDecodeQueueBox::StartAllDecoder()
    {
        for(int i=0;i<streamList.size();i++){
            const EyerAVStream & stream = streamList[i];

            EyerDecodeQueueFFmpeg * decodeQueue = new EyerDecodeQueueFFmpeg(stream);
            decodeQueue->StartDecoder();

            decoderList.push_back(decodeQueue);
        }
        return 0;
    }

    int AVDecodeQueueBox::StopAllDecoder()
    {
        for(int i=0;i<decoderList.size();i++){
            EyerDecodeQueueBase * decodeQueue = decoderList[i];
            if(decodeQueue != nullptr){
                decodeQueue->StopDeocder();
                delete decodeQueue;
                decodeQueue = nullptr;
            }
        }
        decoderList.clear();
        return 0;
    }

    int AVDecodeQueueBox::PutPacket(EyerAVPacket * packet)
    {
        int streamIndex = packet->GetStreamIndex();
        for(int i=0;i<decoderList.size();i++) {
            EyerDecodeQueueBase *decodeQueue = decoderList[i];
            // TODO 此处有优化空间
            EyerAVStream stream = decodeQueue->GetStream();
            if(stream.GetStreamId() == streamIndex){
                decodeQueue->PutAVPacket(packet);
                return 0;
            }
        }
        return -1;
    }

    int AVDecodeQueueBox::GetCacheSize()
    {
        int size = 0;
        for(int i=0;i<decoderList.size();i++) {
            EyerDecodeQueueBase *decodeQueue = decoderList[i];
            size += decodeQueue->GetCacheSize();
        }
        return size;
    }
}