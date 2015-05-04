/**
 * \brief Capture video stream from RTSP stream using avcodec library
 */
#include <QtCore/QRegExp>
#include <QtCore/QString>

#include "global.h"
#include "stdint.h"
#include "stdio.h"
#include "rtspCapture.h"

RTSPCapture::RTSPCapture(QString const &params):
       mName(params.toStdString())
     , mFormatContext(NULL)
     , mCodecContext(NULL)
     , mCodec(NULL)
     , mIsPaused(false)
     , mFrame(NULL)
     , spin(this)
     , fcb(this)
{
    SYNC_PRINT(("RTSPCapture::RTSPCapture(%s): called\n", params.toLatin1().constData()));
    SYNC_PRINT(("Registering the codecs...\n"));

    av_register_all();
    avformat_network_init();

    SYNC_PRINT(("RTSPCapture::RTSPCapture(): exited\n"));
}

RTSPCapture::~RTSPCapture()
{
    SYNC_PRINT(("RTSPCapture::~RTSPCapture(): called\n"));

    av_free(mFrame);
    avcodec_close(mCodecContext);
    avformat_close_input(&mFormatContext);

    SYNC_PRINT(("RTSPCapture::~RTSPCapture(): exited\n"));
}

ImageCaptureInterface::CapErrorCode RTSPCapture::initCapture()
{
    SYNC_PRINT(("RTSPCapture::initCapture(): called\n"));

    int res;
    res = avformat_open_input(&mFormatContext, mName.c_str(), NULL, NULL);
    if (res < 0) {
        SYNC_PRINT(("RTSPCapture::initCapture(): failed to open file"));
        return ImageCaptureInterface::FAILURE;
    }

    res = avformat_find_stream_info(mFormatContext, NULL);
    if (res < 0) {
        SYNC_PRINT(("RTSPCapture::initCapture(): Unable to find stream info\n"));
        return ImageCaptureInterface::FAILURE;
    }

    SYNC_PRINT(("Stream seem to have %d streams\n", mFormatContext->nb_streams));

    // Dump information about file onto standard error
    av_dump_format(mFormatContext, 0, mName.c_str(), 0);

    // Find the first video stream
    for (mVideoStream = 0; mVideoStream < mFormatContext->nb_streams; mVideoStream++) {
        if (mFormatContext->streams[mVideoStream]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            break;
        }
    }

    if (mVideoStream == mFormatContext->nb_streams) {
        SYNC_PRINT(("RTSPCapture::initCapture(): Unable to find video stream among %d streams\n", mFormatContext->nb_streams));
        return ImageCaptureInterface::FAILURE;
    }

    SYNC_PRINT(("RTSPCapture::initCapture(): Video Stream found\n"));
    mCodecContext = mFormatContext->streams[mVideoStream]->codec;
    mCodec = avcodec_find_decoder(mCodecContext->codec_id);
    res = avcodec_open2(mCodecContext, mCodec, NULL);
    if (res < 0) {
        SYNC_PRINT(("RTSPCapture::initCapture(): Unable to open codec\n"));
        return ImageCaptureInterface::FAILURE;
    }
    SYNC_PRINT(("RTSPCapture::initCapture(): Video codec found\n"));

    mFrame = avcodec_alloc_frame();

    SYNC_PRINT(("RTSPCapture::initCapture(): exited\n"));
    return ImageCaptureInterface::SUCCESS;
}

ImageCaptureInterface::FramePair RTSPCapture::getFrame()
{
    CaptureStatistics  stats;
    PreciseTimer start = PreciseTimer::currentTime();

    FramePair result = fcb.dequeue();

    stats.values[CaptureStatistics::DECODING_TIME] = start.usecsToNow();

    if (mLastFrameTime.usecsTo(PreciseTimer()) != 0)
    {
        stats.values[CaptureStatistics::INTERFRAME_DELAY] = mLastFrameTime.usecsToNow();
    }
    mLastFrameTime = PreciseTimer::currentTime();
    stats.values[CaptureStatistics::DATA_SIZE] = 0;
    emit newStatisticsReady(stats);

    if (!mIsPaused)
    {        
        frame_data_t frameData;
        frameData.timestamp = fcb.secondFrameTimestamp();
        //SYNC_PRINT(("RTSPCapture::getFrame(): sending notification ts = %d\n", frameData.timestamp));
        notifyAboutNewFrame(frameData);
    } else {
        SYNC_PRINT(("RTSPCapture::getFrame(): Paused\n"));
    }

    return result;
}

ImageCaptureInterface::CapErrorCode RTSPCapture::startCapture()
{
    SYNC_PRINT(("RTSPCapture::startCapture(): called\n"));
    mIsPaused = false;

    frame_data_t frameData;
    frameData.timestamp = 0;
    SYNC_PRINT(("RTSPCapture::startCapture(): sending notification\n"));
    notifyAboutNewFrame(frameData);

    spin.start();

    SYNC_PRINT(("RTSPCapture::startCapture(): exited\n"));
    return ImageCaptureInterface::SUCCESS;
}

ImageCaptureInterface::CapErrorCode RTSPCapture::pauseCapture()
{
    return ImageCaptureInterface::FAILURE;
}

ImageCaptureInterface::CapErrorCode RTSPCapture::nextFrame()
{
    frame_data_t frameData;
    frameData.timestamp = fcb.firstFrameTimestamp();

    SYNC_PRINT(("RTSPCapture::nextFrame(): sending notification ts = %" PRIu64 "\n", frameData.timestamp));
    notifyAboutNewFrame(frameData);
    return ImageCaptureInterface::SUCCESS;
}

bool RTSPCapture::supportPause()
{
    return false;
}

void RTSPCapture::SpinThread::run()
{
    SYNC_PRINT(("RTSPCapture::SpinThread::run(): starting\n"));

    AVFrame* frame = mInstance->mFrame;
    AVPacket packet = mInstance->mPacket;
    FramePair result(NULL, NULL);
    int res;
    while (true)
    {
        av_init_packet(&packet);
        while ( (res = av_read_frame(mInstance->mFormatContext, &packet) ) >= 0)
        {
            if (packet.stream_index == mInstance->mVideoStream)
            {
                int frame_finished;
                avcodec_decode_video2(mInstance->mCodecContext,
                                      frame,
                                      &frame_finished,
                                      &packet);

                if (frame_finished) {
//                    SYNC_PRINT(("RTSPCapture::SpinThread::run(): Frame ready\n"));
                    av_free_packet(&packet);
                    break;
                }
            }
            // Free the packet that was allocated by av_read_frame
            av_free_packet(&packet);
        }

        if (res >= 0)
        {
            if (frame->format != PIX_FMT_YUV420P && frame->format != PIX_FMT_YUVJ420P)
            {
                SYNC_PRINT(("RTSPCapture::SpinThread::run(): Not supported format %d\n", frame->format));
                continue;
            }

            result.rgbBufferLeft = new RGB24Buffer(frame->height, frame->width);
            result.bufferLeft    = new G12Buffer  (frame->height, frame->width);
            for (int i = 0; i < frame->height; i++)
            {
                for (int j = 0; j < frame->width; j++)
                {
                    uint8_t y = (frame->data[0])[i * frame->linesize[0] + j];

                    uint8_t u = (frame->data[1])[(i / 2) * frame->linesize[1] + (j / 2)];
                    uint8_t v = (frame->data[2])[(i / 2) * frame->linesize[2] + (j / 2)];

                    result.rgbBufferLeft->element(i,j) = RGBColor::FromYUV(y,u,v);
                    result.bufferLeft   ->element(i,j) = (int)y << 4;
                }
            }

            result.rgbBufferRight = new RGB24Buffer(result.rgbBufferLeft);
            result.bufferRight = new G12Buffer(result.bufferLeft);
        }

//        int64_t timestamp = av_frame_get_best_effort_timestamp(frame);
        int64_t timestamp = PreciseTimer::currentTime().usec();

        result.leftTimeStamp  = timestamp;
        result.rightTimeStamp = timestamp;

        if (mFirstFrame)
        {
            mFirstFrame = false;
            continue;
        }

        mInstance->fcb.enqueue(result);
    }
}

uint64_t RTSPCapture::FrameCircularBuffer::firstFrameTimestamp()
{
    uint64_t result;
    mReadyFrames.acquire();
    result = mFrames.head().leftTimeStamp;
    mReadyFrames.release();
    return result;
}

uint64_t RTSPCapture::FrameCircularBuffer::secondFrameTimestamp()
{
    uint64_t result;
    mReadyFrames.acquire(2);
    result = mFrames.at(1).leftTimeStamp;
    mReadyFrames.release(2);
    return result;
}

ImageCaptureInterface::FramePair RTSPCapture::FrameCircularBuffer::dequeue()
{
    mReadyFrames.acquire();
    FramePair result = mFrames.dequeue();
    mFreeFrames.release();
    return result;
}

void RTSPCapture::FrameCircularBuffer::enqueue(const FramePair& frame)
{
    mFreeFrames.acquire();
    mFrames.enqueue(frame);
    mReadyFrames.release();
}