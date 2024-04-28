#include "audio_element.h"
#include "audio_pipeline.h"
#include "mp3_decoder.h"

struct i2s_stream_handle_t {
    int volume;
    int channel;
    audio_element_handle_t i2s_stream_writer;
    audio_element_handle_t http_stream_reader;
    audio_element_handle_t mp3_decoder;
    audio_pipeline_handle_t pipeline;
};

void i2s_stream_start(void);
int i2s_volume_up(void);
int i2s_volume_down(void);
int i2s_channel_next(void);
int i2s_channel_prev(void);
