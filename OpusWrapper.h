#pragma once
#include <opus.h>

#ifdef OPUS_EXPORTS
#define OPUS_API extern "C" __declspec(dllexport)
#else
#define OPUS_API extern "C" __declspec(dllimport)
#endif

// 编码器接口
OPUS_API int myOpus_encoder_create(int sample_rate, int channels, int* error);
OPUS_API int myOpus_encode(int encoder_handle, const float* pcm, int frame_size, unsigned char* output, int max_size);
OPUS_API void myOpus_encoder_destroy(int encoder_handle);

// 解码器接口
OPUS_API int myOpus_decoder_create(int sample_rate, int channels, int* error);
OPUS_API int myOpus_decode(int decoder_handle, const unsigned char* data, int len, float* pcm, int frame_size);
OPUS_API void myOpus_decoder_destroy(int decoder_handle);