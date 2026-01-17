#include "AudioReader.h"

AudioReader::AudioReader()
{
	audioBuffer.resize(1024 * 1024 * 4);
}

AudioReader::~AudioReader()
{
	Close();
}

bool AudioReader::Open(const char* filename)
{
	av_log_set_level(AV_LOG_ERROR);
	
	avFormatCTX = avformat_alloc_context();
	if (!avFormatCTX)
		return false;

	if (avformat_open_input(&avFormatCTX, filename, nullptr, nullptr) != 0)
		return false;

	if (avformat_find_stream_info(avFormatCTX, nullptr) < 0)
		return false;

	audioStreamIndex = -1;
	AVCodecParameters* avCodecParams = nullptr;
	const AVCodec* avCodec = nullptr;

	for (int i = 0; i < avFormatCTX->nb_streams; i++)
	{
		avCodecParams = avFormatCTX->streams[i]->codecpar;
		avCodec = avcodec_find_decoder(avCodecParams->codec_id);

		if (!avCodec)
			continue;

		if (avCodecParams->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioStreamIndex = i;
			timeBase = avFormatCTX->streams[i]->time_base;
			
			AVStream* avStream = avFormatCTX->streams[audioStreamIndex];
			if (avStream->duration != AV_NOPTS_VALUE)
			{
				duration = avStream->duration * av_q2d(avStream->time_base);
			}
			else if (avFormatCTX->duration != AV_NOPTS_VALUE)
			{
				duration = avFormatCTX->duration / (double)AV_TIME_BASE;
			}
			
			sampleRate = avCodecParams->sample_rate;
			channels = avCodecParams->ch_layout.nb_channels;
			
			break;
		}
	}

	if (audioStreamIndex == -1)
		return false;

	avCodecCTX = avcodec_alloc_context3(avCodec);
	if (!avCodecCTX)
		return false;

	if (avcodec_parameters_to_context(avCodecCTX, avCodecParams) < 0)
		return false;

	if (avcodec_open2(avCodecCTX, avCodec, nullptr) < 0)
		return false;

	avFrame = av_frame_alloc();
	avPacket = av_packet_alloc();

	if (!avFrame || !avPacket)
		return false;

	AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_STEREO;
	if (channels == 1)
		out_ch_layout = AV_CHANNEL_LAYOUT_MONO;
	
	swr_alloc_set_opts2(&swrContext,
						&out_ch_layout,
						AV_SAMPLE_FMT_FLT,
						sampleRate,
						&avCodecCTX->ch_layout,
						avCodecCTX->sample_fmt,
						avCodecCTX->sample_rate,
						0,
						nullptr);
	
	if (!swrContext || swr_init(swrContext) < 0)
		return false;

	deviceConfig = ma_device_config_init(ma_device_type_playback);
	deviceConfig.playback.format = ma_format_f32;
	deviceConfig.playback.channels = channels;
	deviceConfig.sampleRate = sampleRate;
	deviceConfig.dataCallback = AudioCallback;
	deviceConfig.pUserData = this;

	if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS)
		return false;

	deviceInitialized = true;
	return true;
}

void AudioReader::AudioCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	AudioReader* reader = (AudioReader*)pDevice->pUserData;
	float* output = (float*)pOutput;
	
	std::lock_guard<std::mutex> lock(reader->bufferMutex);
	
	size_t bytesNeeded = frameCount * reader->channels * sizeof(float);
	size_t bytesAvailable = reader->bufferWritePos - reader->bufferReadPos;
	
	if (bytesAvailable >= bytesNeeded)
	{
		memcpy(output, &reader->audioBuffer[reader->bufferReadPos], bytesNeeded);
		reader->bufferReadPos += bytesNeeded;
	}
	else
	{
		if (bytesAvailable > 0)
			memcpy(output, &reader->audioBuffer[reader->bufferReadPos], bytesAvailable);
		memset((uint8_t*)output + bytesAvailable, 0, bytesNeeded - bytesAvailable);
		reader->bufferReadPos += bytesAvailable;
	}
	
	if (reader->bufferReadPos >= reader->audioBuffer.size() / 2)
	{
		size_t remaining = reader->bufferWritePos - reader->bufferReadPos;
		if (remaining > 0)
			memmove(&reader->audioBuffer[0], &reader->audioBuffer[reader->bufferReadPos], remaining);
		reader->bufferWritePos = remaining;
		reader->bufferReadPos = 0;
	}
}

bool AudioReader::ReadAndDecodeAudioFrame()
{
	while (true)
	{
		int ret = av_read_frame(avFormatCTX, avPacket);
		if (ret < 0)
			return false;

		if (avPacket->stream_index != audioStreamIndex)
		{
			av_packet_unref(avPacket);
			continue;
		}

		int response = avcodec_send_packet(avCodecCTX, avPacket);
		av_packet_unref(avPacket);

		if (response < 0)
			return false;

		response = avcodec_receive_frame(avCodecCTX, avFrame);

		if (response == AVERROR(EAGAIN))
			continue;
		else if (response == AVERROR_EOF || response < 0)
			return false;

		break;
	}

	if (avFrame->pts != AV_NOPTS_VALUE)
		currentPts = avFrame->pts * av_q2d(timeBase);

	int maxOutSamples = av_rescale_rnd(avFrame->nb_samples, 
										sampleRate, 
										avCodecCTX->sample_rate, 
										AV_ROUND_UP);
	
	uint8_t** outBuffer = nullptr;
	int outLinesize = 0;
	
	av_samples_alloc_array_and_samples(&outBuffer, 
										&outLinesize, 
										channels, 
										maxOutSamples, 
										AV_SAMPLE_FMT_FLT, 
										0);

	int outSamples = swr_convert(swrContext,
								  outBuffer,
								  maxOutSamples,
								  (const uint8_t**)avFrame->data,
								  avFrame->nb_samples);

	if (outSamples > 0)
	{
		size_t dataSize = outSamples * channels * sizeof(float);
		
		std::lock_guard<std::mutex> lock(bufferMutex);
		
		if (bufferWritePos + dataSize < audioBuffer.size())
		{
			memcpy(&audioBuffer[bufferWritePos], outBuffer[0], dataSize);
			bufferWritePos += dataSize;
		}
	}

	if (outBuffer)
	{
		av_freep(&outBuffer[0]);
		av_freep(&outBuffer);
	}

	return true;
}

void AudioReader::FillBuffer()
{
	if (!isPlaying && !isPrefilling)
		return;
	
	size_t available;
	{
		std::lock_guard<std::mutex> lock(bufferMutex);
		available = bufferWritePos - bufferReadPos;
	}
	
	size_t targetSize = audioBuffer.size() / 2;
	
	while (available < targetSize)
	{
		if (!ReadAndDecodeAudioFrame())
			break;
			
		std::lock_guard<std::mutex> lock(bufferMutex);
		available = bufferWritePos - bufferReadPos;
	}
}

bool AudioReader::PrefillBuffer()
{
	isPrefilling = true;
	
	for (int i = 0; i < 20; i++)
	{
		if (!ReadAndDecodeAudioFrame())
		{
			isPrefilling = false;
			return false;
		}
	}
	
	isPrefilling = false;
	return true;
}

bool AudioReader::Play()
{
	if (!deviceInitialized)
		return false;
	
	isPlaying = true;
	
	if (ma_device_start(&device) != MA_SUCCESS)
	{
		isPlaying = false;
		return false;
	}
	
	return true;
}

void AudioReader::Pause()
{
	isPlaying = false;
	if (deviceInitialized)
		ma_device_stop(&device);
}

void AudioReader::Stop()
{
	isPlaying = false;
	if (deviceInitialized)
		ma_device_stop(&device);
	
	std::lock_guard<std::mutex> lock(bufferMutex);
	bufferReadPos = 0;
	bufferWritePos = 0;
}

bool AudioReader::Seek(double targetTime)
{
	if (!avFormatCTX || audioStreamIndex < 0)
		return false;

	bool wasPlaying = isPlaying;
	Pause();
	
	std::lock_guard<std::mutex> lock(bufferMutex);
	bufferReadPos = 0;
	bufferWritePos = 0;

	int64_t timestamp = (int64_t)(targetTime / av_q2d(timeBase));

	if (av_seek_frame(avFormatCTX, audioStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD) < 0)
		return false;

	avcodec_flush_buffers(avCodecCTX);
	currentPts = targetTime;

	return true;
}

double AudioReader::GetCurrentTime() const
{
	return currentPts;
}

void AudioReader::SetMasterTime(double time)
{
	masterTime = time;
}

double AudioReader::GetAudioLatency() const
{
	if (!deviceInitialized)
		return 0.0;
	
	size_t bytesInBuffer = bufferWritePos - bufferReadPos;
	size_t samplesInBuffer = bytesInBuffer / (channels * sizeof(float));
	return (double)samplesInBuffer / (double)sampleRate;
}

void AudioReader::Close()
{
	Stop();
	
	if (deviceInitialized)
	{
		ma_device_uninit(&device);
		deviceInitialized = false;
	}

	if (swrContext)
	{
		swr_free(&swrContext);
		swrContext = nullptr;
	}

	if (avPacket)
	{
		av_packet_free(&avPacket);
		avPacket = nullptr;
	}

	if (avFrame)
	{
		av_frame_free(&avFrame);
		avFrame = nullptr;
	}

	if (avCodecCTX)
	{
		avcodec_free_context(&avCodecCTX);
		avCodecCTX = nullptr;
	}

	if (avFormatCTX)
	{
		avformat_close_input(&avFormatCTX);
		avFormatCTX = nullptr;
	}
}
