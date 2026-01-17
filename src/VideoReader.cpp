#include "VideoReader.h"

VideoReader::VideoReader() {}

VideoReader::~VideoReader()
{
  Close();
}

bool VideoReader::Open(const char* filename)
{
  // Suppress unnecessary FFmpeg warnings
  av_log_set_level(AV_LOG_ERROR);
  
  avFormatCTX = avformat_alloc_context();
  if (!avFormatCTX)
	return false;

  if (avformat_open_input(&avFormatCTX, filename, nullptr, nullptr) != 0)
	return false;

  if (avformat_find_stream_info(avFormatCTX, nullptr) < 0)
	return false;

  videoStreamIndex = -1;
  AVCodecParameters* avCodecParams = nullptr;
  const AVCodec* avCodec = nullptr;
  AVStream* avStream;

  for (int i = 0; i < avFormatCTX->nb_streams; i++)
  {
	avCodecParams = avFormatCTX->streams[i]->codecpar;
	avCodec = avcodec_find_decoder(avCodecParams->codec_id);

	if (!avCodec)
	  continue;

	if (avCodecParams->codec_type == AVMEDIA_TYPE_VIDEO)
	{
	  videoStreamIndex = i;
	  width  = avCodecParams->width;
	  height = avCodecParams->height;
	  timeBase = avFormatCTX->streams[i]->time_base;
	  avStream = avFormatCTX->streams[videoStreamIndex];
	  totelFrames = avFormatCTX->streams[videoStreamIndex]->nb_frames;

	  if (avStream->duration != AV_NOPTS_VALUE)
	  {
		duration = avStream->duration * av_q2d(avStream->time_base);
	  }
	  else if (avFormatCTX->duration != AV_NOPTS_VALUE)
	  {
		duration = avFormatCTX->duration / (double)AV_TIME_BASE;
	  }
	  else
	  {
		duration = 0.0;
	  }

	  break;
	}
  }

  if (videoStreamIndex == -1)
	return false;

  avCodecCTX = avcodec_alloc_context3(avCodec);
  if (!avCodecCTX)
	return false;

  if (avcodec_parameters_to_context(avCodecCTX, avCodecParams) < 0)
	return false;

  // Enable error concealment for H264
  avCodecCTX->error_concealment = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
  avCodecCTX->err_recognition = AV_EF_CAREFUL;

  if (avcodec_open2(avCodecCTX, avCodec, nullptr) < 0)
	return false;

  avFrame  = av_frame_alloc();
  avPacket = av_packet_alloc();

  if (!avFrame || !avPacket)
	return false;

  return true;
}

bool VideoReader::ReadFrame(uint8_t* frameBuffer, int64_t* pts)
{
  int response;

  while (true)
  {
	int ret = av_read_frame(avFormatCTX, avPacket);
	if (ret < 0)
	{
	  return false;
	}

	if (avPacket->stream_index != videoStreamIndex)
	{
	  av_packet_unref(avPacket);
	  continue;
	}

	response = avcodec_send_packet(avCodecCTX, avPacket);
	av_packet_unref(avPacket);

	if (response < 0)
	{
	  // Skip corrupted packets
	  continue;
	}

	response = avcodec_receive_frame(avCodecCTX, avFrame);

	if (response == AVERROR(EAGAIN))
	  continue;
	else if (response == AVERROR_EOF)
	  return false;
	else if (response < 0)
	{
	  // Skip corrupted frames
	  continue;
	}

	break;
  }

  *pts = (avFrame->pts != AV_NOPTS_VALUE) ? avFrame->pts : avFrame->best_effort_timestamp;

  if (!swsScalerCTX)
  {
	swsScalerCTX = sws_getContext(
		width,
		height,
		avCodecCTX->pix_fmt,
		width,
		height,
		AV_PIX_FMT_RGB0,
		SWS_BILINEAR,
		nullptr,
		nullptr,
		nullptr
		);
  }
  if (!swsScalerCTX)
	return false;

  uint8_t* dest[4] = { frameBuffer, nullptr, nullptr, nullptr };
  int destLinesizes[4] = { width * 4, 0, 0, 0 };

  sws_scale(swsScalerCTX, avFrame->data, avFrame->linesize, 0, height, dest, destLinesizes);

  return true;
}

bool VideoReader::Seek(double targetTime)
{
  if (!avFormatCTX || videoStreamIndex < 0)
	return false;

  targetTime = glm::clamp(targetTime, 0.0, duration - 0.033);

  int64_t timestamp = (int64_t)(targetTime / av_q2d(timeBase));

  if (av_seek_frame(avFormatCTX, videoStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD) < 0)
	return false;

  avcodec_flush_buffers(avCodecCTX);

  int64_t pts;
  uint8_t tempBuffer[width * height * 4];
  
  while (true)
  {
	if (!ReadFrame(tempBuffer, &pts))
	  return false;
	
	double frameTime = pts * av_q2d(timeBase);
	
	if (frameTime >= targetTime - 0.001)
	  break;
  }

  return true;
}

double VideoReader::GetDuration() const
{
  return duration;
}

void VideoReader::Close()
{
  if (swsScalerCTX)
  {
	sws_freeContext(swsScalerCTX);
	swsScalerCTX = nullptr;
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
