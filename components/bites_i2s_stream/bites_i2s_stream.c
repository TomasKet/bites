

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "sdkconfig.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "http_stream.h"
#include "i2s_stream.h"
#include "esp_netif.h"

#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "board.h"
#include "bites_i2s_stream.h"
#include "storas.h"

static const char *TAG = "bites_i2s_stream";

const char *stream_uri_preset[] = {
    "http://stream.palanga.live/palanga128.mp3",
    "http://stream-relay-geo.ntslive.net/stream",
    "http://stream-relay-geo.ntslive.net/stream2",
    "http://aaja.radiocult.fm/stream",
    "http://aaja-2.radiocult.fm/stream",
};
const int stream_uri_preset_sz = sizeof(stream_uri_preset) / sizeof(stream_uri_preset[0]);

struct i2s_stream_handle_t i2s;

static void i2s_stream_restart(void);

void i2s_stream_start(void)
{
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    ESP_LOGI(TAG, "[ 1 ] Start audio codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

    ESP_LOGI(TAG, "[2.0] Create audio pipeline for playback");
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    i2s.pipeline = audio_pipeline_init(&pipeline_cfg);
    mem_assert(i2s.pipeline);

    ESP_LOGI(TAG, "[2.1] Create http stream to read data");
    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    i2s.http_stream_reader = http_stream_init(&http_cfg);

    ESP_LOGI(TAG, "[2.2] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_cfg.use_alc = true;
    i2s.i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    ESP_LOGI(TAG, "[2.3] Create mp3 decoder to decode mp3 file");
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    i2s.mp3_decoder = mp3_decoder_init(&mp3_cfg);

    ESP_LOGI(TAG, "[2.4] Register all elements to audio pipeline");
    audio_pipeline_register(i2s.pipeline, i2s.http_stream_reader, "http");
    audio_pipeline_register(i2s.pipeline, i2s.mp3_decoder,        "mp3");
    audio_pipeline_register(i2s.pipeline, i2s.i2s_stream_writer,  "i2s");

    ESP_LOGI(TAG, "[2.5] Link it together http_stream-->mp3_decoder-->i2s_stream-->[codec_chip]");
    const char *link_tag[3] = {"http", "mp3", "i2s"};
    audio_pipeline_link(i2s.pipeline, &link_tag[0], 3);

    ESP_LOGI(TAG, "[2.6] Set up  uri (http as http_stream, mp3 as mp3 decoder, and default output is i2s)");
    audio_element_set_uri(i2s.http_stream_reader, stream_uri_preset[i2s.channel]);


    // Example of using an audio event -- START
    ESP_LOGI(TAG, "[ 4 ] Set up  event listener");
    audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
    audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

    ESP_LOGI(TAG, "[4.1] Listening event from all elements of pipeline");
    audio_pipeline_set_listener(i2s.pipeline, evt);

    ESP_LOGI(TAG, "[ 5 ] Start audio_pipeline");
    audio_pipeline_run(i2s.pipeline);
    while (1) {
        audio_event_iface_msg_t msg;
        esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "[ * ] Event interface error : %d", ret);
            continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.source == (void *) i2s.mp3_decoder
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO) {
            audio_element_info_t music_info = {0};
            audio_element_getinfo(i2s.mp3_decoder, &music_info);

            ESP_LOGI(TAG, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
                     music_info.sample_rates, music_info.bits, music_info.channels);

            i2s_stream_set_clk(i2s.i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
            continue;
        }

        /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
        /*
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) i2s.http_stream_reader
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int) msg.data == AEL_STATUS_ERROR_OPEN) {
            ESP_LOGW(TAG, "[ * ] Restart stream");
            i2s_stream_restart();
            continue;
        }
        */
    }
    // Example of using an audio event -- END

    ESP_LOGI(TAG, "[ 6 ] Stop audio_pipeline");
    audio_pipeline_stop(i2s.pipeline);
    audio_pipeline_wait_for_stop(i2s.pipeline);
    audio_pipeline_terminate(i2s.pipeline);
    /* Terminate the pipeline before removing the listener */
    audio_pipeline_unregister(i2s.pipeline, i2s.http_stream_reader);
    audio_pipeline_unregister(i2s.pipeline, i2s.i2s_stream_writer);
    audio_pipeline_unregister(i2s.pipeline, i2s.mp3_decoder);

    audio_pipeline_remove_listener(i2s.pipeline);

    /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
    audio_event_iface_destroy(evt);

    /* Release all resources */
    audio_pipeline_deinit(i2s.pipeline);
    audio_element_deinit(i2s.http_stream_reader);
    audio_element_deinit(i2s.i2s_stream_writer);
    audio_element_deinit(i2s.mp3_decoder);
}

int i2s_volume_up(void)
{
    if (i2s_alc_volume_set(i2s.i2s_stream_writer, i2s.volume + 5)) {
        return -1;
    }
    i2s.volume += 5;
    ESP_LOGI(TAG, "Volume=%d", i2s.volume);
    return 0;
}

int i2s_volume_down(void)
{
    if (i2s_alc_volume_set(i2s.i2s_stream_writer, i2s.volume - 5)) {
        return -1;
    }
    i2s.volume -= 5;
    ESP_LOGI(TAG, "Volume=%d", i2s.volume);
    return 0;
}

int i2s_channel_next(void)
{
    int channel_next = 0;
    const char *uri_next = NULL;

    // current channel = preset_last  = next is custom
    if (i2s.channel == stream_uri_preset_sz - 1) {
        channel_next = i2s.channel + 1;
        uri_next = stream_uri_custom;
    // current channel = last = custom
    } else if (i2s.channel == stream_uri_preset_sz) {
        channel_next = 0;
        uri_next = stream_uri_preset[channel_next];
    } else {
        channel_next = i2s.channel + 1;
        uri_next = stream_uri_preset[channel_next];
    }

    if (audio_element_set_uri(i2s.http_stream_reader, uri_next)) {
        return -1;
    }
    i2s_stream_restart();
    i2s.channel = channel_next;
    ESP_LOGI(TAG, "Channel=%d", i2s.channel);
    return 0;
}

int i2s_channel_prev(void)
{
    int channel_prev = 0;
    const char *uri_prev = NULL;

    // current channel = 0 = prev is custom
    if (i2s.channel == 0) {
        channel_prev = stream_uri_preset_sz;
        uri_prev = stream_uri_custom;
    // current channel = custom = prev is preset_last
    } else if (i2s.channel == stream_uri_preset_sz) {
        channel_prev = stream_uri_preset_sz - 1;
        uri_prev = stream_uri_preset[channel_prev];
    } else {
        channel_prev = i2s.channel - 1;
        uri_prev = stream_uri_preset[channel_prev];
    }

    if (audio_element_set_uri(i2s.http_stream_reader, uri_prev)) {
        return -1;
    }
    i2s_stream_restart();
    i2s.channel = channel_prev;
    ESP_LOGI(TAG, "Channel=%d", i2s.channel);
    return 0;
}

static void i2s_stream_restart(void)
{
    audio_pipeline_stop(i2s.pipeline);
    audio_pipeline_wait_for_stop(i2s.pipeline);
    audio_element_reset_state(i2s.mp3_decoder);
    audio_element_reset_state(i2s.i2s_stream_writer);
    audio_pipeline_reset_ringbuffer(i2s.pipeline);
    audio_pipeline_reset_items_state(i2s.pipeline);
    audio_pipeline_run(i2s.pipeline);
}