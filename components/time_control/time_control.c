#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"
#include <time.h>

#include "time_control.h"

static const char *TAG = "time_control";

static void time_sync_notification_cb(struct timeval *tv);
static void set_timezone(void);

void sntp_sync(void)
{
    ESP_LOGI(TAG, "Initializing and starting SNTP");

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");

    config.sync_cb = time_sync_notification_cb;

    esp_netif_sntp_init(&config);

    // wait for time to be set
    int retry = 0;
    const int retry_count = 15;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }

    set_timezone();

    esp_netif_sntp_deinit();
}

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void set_timezone(void)
{
    char strftime_buf[64];
    time_t now;
    struct tm timeinfo;

    // Set timezone to Vilnius Standard Time
    setenv("TZ", "EET-2EEST-3,M3.5.0/03:00:00,M10.5.0/04:00:00", 1);
    tzset();
    
    // update 'now' variable with current time
    time(&now);
    localtime_r(&now, &timeinfo);

    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Vilnius is: %s", strftime_buf);
}