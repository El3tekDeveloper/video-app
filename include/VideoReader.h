#ifndef VIDEOREADER_H
#define VIDEOREADER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
}

class VideoReader
{
public:
    VideoReader();
    ~VideoReader();

    bool Open(const char* filename);
    bool ReadFrame(uint8_t* frameBuffer, int64_t* pts);
    bool Seek(double targetTime);
    void Close();

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    AVRational GetTimeBase() const { return timeBase; }
    double GetDuration() const;

private:
    AVFormatContext* avFormatCTX = nullptr;
    AVCodecContext* avCodecCTX   = nullptr;
    AVFrame* avFrame             = nullptr;
    AVPacket* avPacket           = nullptr;
    SwsContext* swsScalerCTX     = nullptr;

    int videoStreamIndex = -1;

    int width = 0;
    int height = 0;
    
    double duration = 0.0;
    AVRational timeBase;
};

#endif