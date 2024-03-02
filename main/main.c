#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include <string.h>
#include "mdns.h"

#include "wifi_ap.h"
#include "wifi_sta.h"
#include "http_server.h"

static const char *TAG = "main";

static void init_nvs(void);
static void init_mdns(void);

char ssid[32] = { 0 };
char pass[32] = { 0 };

void app_main(void)
{
    init_nvs();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    init_mdns();

    if (strlen(ssid) > 0 && strlen(pass) > 0)
        wifi_sta_init(ssid, pass);
    else
        wifi_ap_init();

    http_server_init();
}

static void init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGE(TAG, "nvs flash erase");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    nvs_handle_t my_handle;
    ret = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (ret != ESP_OK)
        ESP_LOGE(TAG, "nvs open fail");

    size_t len = 32;
    nvs_get_str(my_handle, "wifi_ssid", ssid, &len);
    len = 32;
    nvs_get_str(my_handle, "wifi_password", pass, &len);

    ESP_LOGI(TAG,"NVS ssid:%s pass:%s\n", ssid, pass);

    nvs_close(my_handle);
}

static void init_mdns(void)
{
    char *hostname = "esp32";
    //initialize mDNS
    ESP_ERROR_CHECK( mdns_init() );
    //set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK( mdns_hostname_set(hostname) );
    ESP_LOGI(TAG, "mdns hostname set to: [%s]", hostname);
}