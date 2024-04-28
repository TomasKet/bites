#include "esp_log.h"
#include "nvs_flash.h"

#include "storas.h"

static const char *TAG = "storas";

char ssid[32];
char pass[32];
char stream_uri_custom[100];

nvs_handle_t storas_handle;

int init_storas(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGE(TAG, "nvs flash erase");
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            return ret;
        }
        ret = nvs_flash_init();
        if (ret != ESP_OK) {
            return ret;
        }
    }
    if (ret != ESP_OK) {
        return ret;
    }

    ret = nvs_open("storage", NVS_READWRITE, &storas_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    size_t len = 32;
    nvs_get_str(storas_handle, "wifi_ssid", ssid, &len);
    len = 32;
    nvs_get_str(storas_handle, "wifi_password", pass, &len);
    len = sizeof(stream_uri_custom);
    nvs_get_str(storas_handle, "stream_uri_custom", stream_uri_custom, &len);

    ESP_LOGI(TAG,"NVS ssid:%s pass:%s stream_uri_custom:%s\n", ssid, pass, stream_uri_custom);

    return 0;
}

int restore_defaults(void)
{
    return nvs_flash_erase();
}

int storas_set_str(const char* key, const char* value)
{
    if (nvs_set_str(storas_handle, key, value) != ESP_OK) {
        return -1;
    }
    if (nvs_commit(storas_handle) != ESP_OK) {
        return -1;
    }

    return 0;
}

int storas_set_int(const char* key, int value)
{
    if (nvs_set_i32(storas_handle, key, (int32_t)value) != ESP_OK) {
        return -1;
    }
    if (nvs_commit(storas_handle) != ESP_OK) {
        return -1;
    }
    return 0;
}

int storas_get_int(const char* key, int* out_value)
{
    if (nvs_get_i32(storas_handle, key, (int32_t *)out_value) != ESP_OK) {
        return -1;
    }
    return 0;
}
