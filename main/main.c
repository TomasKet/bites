#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"
#include "driver/gpio.h"

static const char *TAG = "example";

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

#define SECONDS_IN_MIN  60
#define SECONDS_IN_HOUR 3600

static void obtain_time(void);

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}
#define PIN_GPIO 5

void app_main(void)
{
    gpio_reset_pin(PIN_GPIO);
    gpio_set_direction(PIN_GPIO, GPIO_MODE_OUTPUT);
    gpio_hold_dis(PIN_GPIO);
    gpio_set_level(PIN_GPIO, 0);

    time_t now;
    struct tm timeinfo;

    ESP_LOGI(TAG, "Connecting to WiFi and getting time over NTP.");
    obtain_time();
    // update 'now' variable with current time
    time(&now);
    localtime_r(&now, &timeinfo);

    char strftime_buf[64];

    // Set timezone to Vilnius Standard Time
    setenv("TZ", "EET-2EEST-3,M3.5.0/03:00:00,M10.5.0/04:00:00", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Vilnius is: %s", strftime_buf);

    const int hours_start = 6;
    const int minutes_start = 40;

    const int hours_end = 7;
    const int minutes_end = 50;

    time_t start_time = hours_start * 3600 + minutes_start * 60;
    time_t end_time = hours_end * 3600 + minutes_end * 60;    

    time_t time_day = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
    ESP_LOGI(TAG, "time_day %d", (int)(time_day));

    while (start_time <= time_day && time_day <= end_time) {
        gpio_set_level(PIN_GPIO, 1);
        ESP_LOGI(TAG, "ON for %d seconds", (int)(end_time - time_day));
        vTaskDelay(10000 / portTICK_PERIOD_MS);
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

static void obtain_time(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK( esp_event_loop_create_default() );

    ESP_ERROR_CHECK(example_connect());

    ESP_LOGI(TAG, "Initializing and starting SNTP");

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");

    config.sync_cb = time_sync_notification_cb;

    esp_netif_sntp_init(&config);

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 15;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }
    time(&now);
    localtime_r(&now, &timeinfo);

    ESP_ERROR_CHECK( example_disconnect() );
    esp_netif_sntp_deinit();
}
