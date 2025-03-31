#include "OpusWrapper.h"
#include <map>
#include <mutex>

static std::mutex encoder_mutex, decoder_mutex;
static std::map<int, OpusEncoder*> encoders;
static std::map<int, OpusDecoder*> decoders;
static int encoder_handle = 0, decoder_handle = 0;

int myOpus_encoder_create(int sample_rate, int channels, int* error) {
    std::lock_guard<std::mutex> lock(encoder_mutex);

    int opus_error;
    OpusEncoder* enc = opus_encoder_create(sample_rate, channels, OPUS_APPLICATION_VOIP, &opus_error);
    if (opus_error != OPUS_OK) {
        *error = opus_error;
        return -1;
    }

    encoders[++encoder_handle] = enc;
    return encoder_handle;
}

int myOpus_encode(int handle, const float* pcm, int frame_size,
    unsigned char* output, int max_size) {
    auto it = encoders.find(handle);
    if (it == encoders.end()) return OPUS_BAD_ARG;

    return opus_encode_float(it->second, pcm, frame_size, output, max_size);
}

void myOpus_encoder_destroy(int handle) {
    std::lock_guard<std::mutex> lock(encoder_mutex);

    auto it = encoders.find(handle);
    if (it != encoders.end()) {
        opus_encoder_destroy(it->second);
        encoders.erase(it);
    }
}

int myOpus_decoder_create(int sample_rate, int channels, int* error) {
    std::lock_guard<std::mutex> lock(decoder_mutex);

    int opus_error;
    OpusDecoder* dec = opus_decoder_create(sample_rate, channels, &opus_error);
    if (opus_error != OPUS_OK) {
        *error = opus_error;
        return -1;
    }

    decoders[++decoder_handle] = dec;
    return decoder_handle;
}

int myOpus_decode(int handle, const unsigned char* data, int len,
    float* pcm, int frame_size) {
    auto it = decoders.find(handle);
    if (it == decoders.end()) return OPUS_BAD_ARG;

    return opus_decode_float(it->second, data, len, pcm, frame_size, 0);
}

void myOpus_decoder_destroy(int handle) {
    std::lock_guard<std::mutex> lock(decoder_mutex);

    auto it = decoders.find(handle);
    if (it != decoders.end()) {
        opus_decoder_destroy(it->second);
        decoders.erase(it);
    }
}