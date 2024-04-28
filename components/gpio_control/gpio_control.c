#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "esp_log.h"

#include <time.h>

#include "gpio_control.h"
#include "bites_i2s_stream.h"
#include "storas.h"

static const char *TAG = "gpio_control";

struct button btn[BUTTON_COUNT] = { 
    { .gpio = 14 },
    { .gpio = 12 },
};

#define PIN_GPIO 5

#define SECONDS_IN_MIN  60
#define SECONDS_IN_HOUR 3600

static void gpio_task(void *);

void led_control(void)
{
    time_t now;
    struct tm timeinfo;

    gpio_reset_pin(PIN_GPIO);
    gpio_set_direction(PIN_GPIO, GPIO_MODE_OUTPUT);
    gpio_hold_dis(PIN_GPIO);
    gpio_set_level(PIN_GPIO, 0);

    const int hours_start = 6;
    const int minutes_start = 40;

    const int hours_end = 7;
    const int minutes_end = 50;

    time_t start_time = hours_start * 3600 + minutes_start * 60;
    time_t end_time = hours_end * 3600 + minutes_end * 60;

    time(&now);
    localtime_r(&now, &timeinfo);
    time_t time_day = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
    ESP_LOGI(TAG, "time_day %d", (int)(time_day));

    while (start_time <= time_day && time_day <= end_time) {
        gpio_set_level(PIN_GPIO, 1);
        ESP_LOGI(TAG, "ON for %d seconds", (int)(end_time - time_day));
        time(&now);
        localtime_r(&now, &timeinfo);
        time_day = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
    }

    if (time_day >= start_time)
        start_time += 86400;

    int timeout = start_time - time_day;

    int resync_wakeup = SECONDS_IN_MIN * 20;
    if (timeout > resync_wakeup * 3)
        timeout = timeout - resync_wakeup;

    ESP_LOGI(TAG, "Entering deep sleep for %d seconds", timeout);
    gpio_set_level(PIN_GPIO, 0);
    gpio_hold_en(PIN_GPIO);
    gpio_deep_sleep_hold_en();

    esp_deep_sleep(1000000LL * timeout);
}

int gpio_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1 << btn[0].gpio | 1 << btn[1].gpio,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    if (gpio_config(&cfg)) {
		return -1;
    }
	if (xTaskCreate(gpio_task, TAG, 2028, NULL, configMAX_PRIORITIES - 3, NULL) != pdPASS) {
		return -1;
	}

	return 0;
}

static void gpio_task(void *)
{
	while (1) {
        for (int i = 0; i < BUTTON_COUNT; i++) {
            btn[i].is_pressed = !gpio_get_level(btn[i].gpio);
            if (btn[i].is_pressed) {
                btn[i].time_pressed += GPIO_TASK_DELAY;
            }
        }

		if (!btn[0].is_pressed && !btn[1].is_pressed) {
            if (btn[0].time_pressed >= TIMEOUT_DEFAULTS && btn[1].time_pressed >= TIMEOUT_DEFAULTS) {
                if (restore_defaults()) {
    				ESP_LOGI(TAG, "restore defaults failed");
                } else {
                    ESP_LOGI(TAG, "restore defaults");
                    esp_restart();
                }
            } else if (btn[0].time_pressed >= TIMEOUT_RESET && btn[1].time_pressed >= TIMEOUT_RESET) {
				ESP_LOGI(TAG, "reset");
                esp_restart();
            } else if (btn[0].time_pressed >= TIMEOUT_CH_SWITCH || btn[1].time_pressed >= TIMEOUT_CH_SWITCH) {
                if (btn[0].time_pressed >= TIMEOUT_CH_SWITCH) {
                    ESP_LOGI(TAG, "ch++");
                    i2s_channel_next();
                } else if (btn[1].time_pressed >= TIMEOUT_CH_SWITCH) {
                    ESP_LOGI(TAG, "ch--");
                    i2s_channel_prev();
                }
            } else if (btn[0].time_pressed >= GPIO_TASK_DELAY || btn[1].time_pressed >= GPIO_TASK_DELAY) {
                if (btn[0].time_pressed >= GPIO_TASK_DELAY) {
                    ESP_LOGI(TAG, "vol++");
                    i2s_volume_up();
                } else if (btn[1].time_pressed >= GPIO_TASK_DELAY) {
                    ESP_LOGI(TAG, "vol--");
                    i2s_volume_down();
                }
            }
		}

        for (int i = 0; i < BUTTON_COUNT; i++) {
            if (!btn[i].is_pressed) {
                btn[i].time_pressed = 0;
            }
        }
		vTaskDelay(GPIO_TASK_DELAY / portTICK_PERIOD_MS);
	}
}
