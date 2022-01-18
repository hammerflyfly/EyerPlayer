#include "EyerAVFrame.hpp"

#include "EyerCore/EyerCore.hpp"

#include "EyerAVFramePrivate.hpp"
#include "EyerAVPixelFormat.hpp"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

namespace Eyer
{
    EyerAVFrame::EyerAVFrame()
    {
        piml = new EyerAVFramePrivate();
        piml->frame = av_frame_alloc();
    }

    EyerAVFrame::EyerAVFrame(const EyerAVFrame & frame) : EyerAVFrame()
    {
        *this = frame;
    }

    EyerAVFrame::~EyerAVFrame()
    {
        av_frame_unref(piml->frame);
        av_frame_free(&piml->frame);

        for(int i=0;i<piml->data.size();i++){
            free(piml->data[i]);
            piml->data[i] = nullptr;
        }
        piml->data.clear();

        if(piml != nullptr){
            delete piml;
            piml = nullptr;
        }
    }

    EyerAVFrame & EyerAVFrame::operator = (const EyerAVFrame & frame)
    {
        piml->secPTS = frame.piml->secPTS;
        av_frame_copy_props(piml->frame, frame.piml->frame);
        av_frame_copy(piml->frame, frame.piml->frame);
        av_frame_ref(piml->frame, frame.piml->frame);

        return *this;
    }

    int EyerAVFrame::SetPTS(int64_t pts)
    {
        piml->frame->pts = pts;
        return 0;
    }

    int64_t EyerAVFrame::GetPTS()
    {
        return piml->frame->pts;
    }

    double EyerAVFrame::GetSecPTS()
    {
        return piml->secPTS;
    }

    int EyerAVFrame::GetWidth()
    {
        return piml->frame->width;
    }

    int EyerAVFrame::GetHeight()
    {
        return piml->frame->height;
    }

    int EyerAVFrame::SetVideoData420P(unsigned char * _y, unsigned char * _u, unsigned char * _v, int _width, int _height)
    {
        piml->frame->format = AVPixelFormat::AV_PIX_FMT_YUV420P;
        piml->frame->width = _width;
        piml->frame->height = _height;
        piml->frame->extended_data = piml->frame->data;

        av_frame_get_buffer(piml->frame, 16);
        for(int i=0; i<_height; i++){
            memcpy(piml->frame->data[0] + piml->frame->linesize[0] * i, _y + _width * i, _width);
        }

        for(int i=0; i<_height / 2; i++){
            memcpy(piml->frame->data[1] + piml->frame->linesize[1] * i, _u + _width / 2 * i, _width / 2);
            memcpy(piml->frame->data[2] + piml->frame->linesize[2] * i, _v + _width / 2 * i, _width / 2);
        }

        return 0;
    }

    int EyerAVFrame::SetAudioDataFLTP(uint8_t * data, int & offset)
    {
        piml->frame->channel_layout = AV_CH_LAYOUT_STEREO;
        piml->frame->channels = av_get_channel_layout_nb_channels(piml->frame->channel_layout);
        piml->frame->sample_rate = 44100;
        piml->frame->nb_samples = 1152;
        piml->frame->format = AVSampleFormat::AV_SAMPLE_FMT_FLTP;

        // int num_bytes = av_get_bytes_per_sample(AVSampleFormat::AV_SAMPLE_FMT_FLTP);
        // EyerLog("channels: %d\n", piml->frame->channels);

        av_frame_get_buffer(piml->frame, 16);

        int nb_samples = piml->frame->nb_samples;
        float * a = (float *)malloc(nb_samples * sizeof(float));
        for(int i=0;i<nb_samples;i++){
            a[i] = sin(i * (1.0 / nb_samples) * 2 * 3.1415926 * 8);
        }

        memcpy(piml->frame->data[0], a, nb_samples * sizeof(float));
        memcpy(piml->frame->data[1], a, nb_samples * sizeof(float));
        // memset(piml->frame->data[0], 0, piml->frame->linesize[0]);
        // memset(piml->frame->data[1], 0, piml->frame->linesize[0]);

        free(a);

        piml->frame->pts = offset;
        offset += piml->frame->nb_samples;
        // EyerLog("PTS: %d\n", piml->frame->pts);

        // memcpy(piml->frame->data[0], data, );

        return 0;
    }

    int EyerAVFrame::SetAudioDataS16_44100_2_1024 (uint8_t * data)
    {
        piml->frame->format         = AVSampleFormat::AV_SAMPLE_FMT_S16;
        piml->frame->sample_rate    = 44100;
        piml->frame->channels       = 2;
        piml->frame->nb_samples     = 1024;
        piml->frame->channel_layout = av_get_default_channel_layout(piml->frame->channels);

        av_frame_get_buffer(piml->frame, 16);

        // memset(piml->frame->data[0], 0, piml->frame->linesize[0]);
        memcpy(piml->frame->data[0], data, 2 * 2 * 1024);

        return 0;
    }

    int EyerAVFrame::InitAudioData(EyerAVSampleFormat sampleFormat, int sample_rate, int nb_samples, int channels)
    {
        piml->frame->format         = (AVSampleFormat)sampleFormat.id;
        piml->frame->sample_rate    = sample_rate;
        piml->frame->channels       = channels;
        piml->frame->nb_samples     = nb_samples;
        piml->frame->channel_layout = av_get_default_channel_layout(piml->frame->channels);

        av_frame_get_buffer(piml->frame, 16);

        return 0;
    }

    int EyerAVFrame::Resample(EyerAVFrame & dstFrame)
    {
        /*
        av_frame_copy_props(dstFrame.piml->frame, piml->frame);

        dstFrame.piml->frame->channel_layout = AV_CH_LAYOUT_STEREO;
        dstFrame.piml->frame->channels = av_get_channel_layout_nb_channels(dstFrame.piml->frame->channel_layout);
        dstFrame.piml->frame->sample_rate = 44100;
        dstFrame.piml->frame->format = AVSampleFormat::AV_SAMPLE_FMT_FLTP;

        EyerLog("Sample A: %d\n", dstFrame.piml->frame->sample_rate);
        EyerLog("Sample B: %d\n", piml->frame->sample_rate);

        SwrContext * swrCtx = swr_alloc_set_opts(
                NULL,
                dstFrame.piml->frame->channel_layout,
                (AVSampleFormat)dstFrame.piml->frame->format,
                dstFrame.piml->frame->sample_rate,

                piml->frame->channel_layout,
                (AVSampleFormat)piml->frame->format,
                piml->frame->sample_rate,

                0,
                NULL
        );

        int dst_nb_samples = av_rescale_rnd(swr_get_delay(swrCtx, piml->frame->sample_rate) + piml->frame->nb_samples, piml->frame->sample_rate, piml->frame->sample_rate, AVRounding(1));
        EyerLog("dst_nb_samples: %d\n", dst_nb_samples);

        swr_init(swrCtx);

        int ret = swr_convert_frame(swrCtx, dstFrame.piml->frame, piml->frame);

        swr_free(&swrCtx);


        return ret;
         */

        return 0;
    }

    int EyerAVFrame::Scale(EyerAVFrame & frame, const EyerAVPixelFormat format)
    {
        return Scale(frame, format, GetWidth(), GetHeight());
    }

    int EyerAVFrame::Scale(EyerAVFrame & dstFrame, const EyerAVPixelFormat format, const int dstW, const int dstH)
    {
        /*
        AVPixelFormat distFormat = (AVPixelFormat)format.ffmpegId;

        av_frame_copy_props(dstFrame.piml->frame, piml->frame);

        dstFrame.piml->frame->pict_type = AVPictureType::AV_PICTURE_TYPE_NONE;

        dstFrame.piml->frame->format    = distFormat;
        dstFrame.piml->frame->width     = dstW;
        dstFrame.piml->frame->height    = dstH;

        dstFrame.piml->secPTS           = piml->secPTS;

        // uint8_t
        av_frame_get_buffer(dstFrame.piml->frame, 1);

        int srcW = GetWidth();
        int srcH = GetHeight();
        AVPixelFormat srcFormat = (AVPixelFormat)(piml->frame->format);

        SwsContext * swsContext = sws_getContext(srcW, srcH, srcFormat, dstW, dstH, distFormat, SWS_POINT, NULL, NULL, NULL);

        if(swsContext == nullptr){
            return -1;
        }


        sws_scale(
                swsContext,
                piml->frame->data,
                piml->frame->linesize,
                0,
                srcH,

                dstFrame.piml->frame->data,
                dstFrame.piml->frame->linesize
        );

        sws_freeContext(swsContext);
        */

        return 0;
    }

    uint8_t * EyerAVFrame::GetData(int index)
    {
        return piml->frame->data[index];
    }

    int EyerAVFrame::GetLinesize(int index)
    {
        return piml->frame->linesize[index];
    }
}