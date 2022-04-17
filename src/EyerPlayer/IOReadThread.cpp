#include "IOReadThread.hpp"

#include "AVDecodeQueueBox.hpp"

namespace Eyer
{
    IOReadThread::IOReadThread(EyerPlayerContext * _playerContext)
        : playerContext(_playerContext)
    {

    }

    IOReadThread::~IOReadThread()
    {

    }

    void IOReadThread::Run()
    {
        EyerLog("IOReadThread Start\n");
        EyerLog("URL: %s\n", playerContext->url.c_str());

        AVDecodeQueueBox decodeQueueBox;

        reader = new EyerAVReader(playerContext->url);
        int ret = reader->Open();
        if(ret){
            // 打开失败
        }
        else {
            // 打开成功
            int videoStreamIndex = reader->GetVideoStreamIndex();
            int audioStreamIndex = reader->GetAudioStreamIndex();

            decodeQueueBox.ClearAllStream();
            if(videoStreamIndex >= 0){
                EyerAVStream videoStream = reader->GetStream(videoStreamIndex);
                decodeQueueBox.AddStream(videoStream);
            }
            if(audioStreamIndex >= 0){
                EyerAVStream audioStream = reader->GetStream(audioStreamIndex);
                decodeQueueBox.AddStream(audioStream);
            }
            decodeQueueBox.StartAllDecoder();
        }

        while(true){
            if(stopFlag){
                break;
            }
            // 2MB Cache
            if(decodeQueueBox.GetCacheSize() >= 1024 * 1024 * 2){
                continue;
            }

            EyerLog("Cache Size: %d\n", decodeQueueBox.GetCacheSize());

            EyerAVPacket * packet = new EyerAVPacket();
            int ret = reader->Read(packet);
            if(ret){
                // 文件结尾或者出错
                // TODO
                if(packet != nullptr){
                    delete packet;
                    packet = nullptr;
                }
                continue;
            }
            ret = decodeQueueBox.PutPacket(packet);
            if(ret){
                // 插入错误
                if(packet != nullptr){
                    delete packet;
                    packet = nullptr;
                }
            }
        }

        // 结束，把解码器清空
        decodeQueueBox.StopAllDecoder();

        if(reader != nullptr){
            reader->Close();
            delete reader;
            reader = nullptr;
        }

        EyerLog("IOReadThread End\n");
    }
}