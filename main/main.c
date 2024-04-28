#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include <string.h>
#include "mdns.h"

#include "wifi_ap.h"
#include "wifi_sta.h"
#include "http_server.h"
#include "time_control.h"
#include "gpio_control.h"
#include "bites_i2s_stream.h"
#include "storas.h"

static const char *TAG = "main";

static int init_mdns(void);

void app_main(void)
{
    ESP_ERROR_CHECK(init_storas());
    ESP_ERROR_CHECK(gpio_init());

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_ERROR_CHECK(init_mdns());

    if (strlen(ssid) > 0 && strlen(pass) > 0) {
        wifi_sta_init(ssid, pass);
    } else {
        wifi_ap_init();
    }

    http_server_init();

    while(true) {
        if (wifi_sta.is_internet) {
            sntp_sync();
            break;
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    i2s_stream_start();
    // led_control();
}

static int init_mdns(void)
{
    esp_err_t ret = mdns_init();
    if (ret != ESP_OK) {
        return ret;
    }
    ret = mdns_hostname_set(CONFIG_MDNS_HOSTNAME);
    if (ret != ESP_OK) {
        return ret;
    }
    ESP_LOGI(TAG, "mdns hostname set to: [%s]", CONFIG_MDNS_HOSTNAME);
    return 0;
}