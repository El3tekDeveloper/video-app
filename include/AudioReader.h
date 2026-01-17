#ifndef AUDIOREADER_H
#define AUDIOREADER_H

#include <miniaudio/miniaudio.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
}

#include <vector>
#include <mutex>
#include <cstring>

class AudioReader
{
public:
    AudioReader();
    ~AudioReader();

    bool Open(const char* filename);
    void Close();
    
    bool PrefillBuffer();
    
    bool Play();
    void Pause();
    void Stop();
    bool IsPlaying() const { return isPlaying; }
    
    bool Seek(double targetTime);
    double GetCurrentTime() const;
    double GetDuration() const { return duration; }
    
    ma_device& GetDevice() { return device; }

    void SetMasterTime(double time);
    double GetAudioLatency() const;
    
    void FillBuffer();

private:
    static void AudioCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    bool ReadAndDecodeAudioFrame();
    
    AVFormatContext* avFormatCTX = nullptr;
    AVCodecContext* avCodecCTX = nullptr;
    AVFrame* avFrame = nullptr;
    AVPacket* avPacket = nullptr;
    SwrContext* swrContext = nullptr;
    
    ma_device device;
    ma_device_config deviceConfig;
    
    int audioStreamIndex = -1;
    double duration = 0.0;
    AVRational timeBase;
    
    std::vector<uint8_t> audioBuffer;
    size_t bufferReadPos = 0;
    size_t bufferWritePos = 0;
    std::mutex bufferMutex;
    
    bool isPlaying = false;
    bool isPrefilling = false;
    bool deviceInitialized = false;
    
    double currentPts = 0.0;
    double masterTime = 0.0;
    
    int sampleRate = 48000;
    int channels = 2;
};

#endif
