#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "nvs_flash.h"

#include "esp_https_server.h"
#include "esp_tls.h"
#include "sdkconfig.h"

#include <string.h>

#include "http_server.h"

static const char *TAG = "http_server";

static const char *HTML_FORM = \
"<html>\
    <form action=\"/\" method=\"post\">"\
        "<label for=\"ssid\">Local SSID:</label><br>"\
        "<input type=\"text\" id=\"ssid\" name=\"ssid\"><br>"\
        "<label for=\"pass\">Password:</label><br>"\
        "<input type=\"text\" id=\"pass\" name=\"pass\"><br>"\
        "<input type=\"submit\" value=\"Submit\">"\
    "</form>\
</html>";

static int save_params(char *buff);

/* An HTTP GET handler */
static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, HTML_FORM, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler
};

/* An HTTP POST handler */
static esp_err_t echo_post_handler(httpd_req_t *req)
{
    char buf[256] = { 0 };
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf,
                        MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }

        remaining -= ret;

        /* Log data received */
        ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG, "%.*s", ret, buf);
        ESP_LOGI(TAG, "====================================");

        if (save_params(buf))
            ESP_LOGE(TAG, "save params failed");
    }

    // End response
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t echo = {
    .uri       = "/",
    .method    = HTTP_POST,
    .handler   = echo_post_handler,
    .user_ctx  = NULL
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();

    extern const unsigned char servercert_start[] asm("_binary_servercert_pem_start");
    extern const unsigned char servercert_end[]   asm("_binary_servercert_pem_end");
    conf.servercert = servercert_start;
    conf.servercert_len = servercert_end - servercert_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
    extern const unsigned char prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");
    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    esp_err_t ret = httpd_ssl_start(&server, &conf);
    if (ESP_OK != ret) {
        ESP_LOGI(TAG, "Error starting server!");
        return NULL;
    }

    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &echo);
    return server;
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        *server = start_webserver();
    }
}

void http_server_init(void)
{
    static httpd_handle_t server;
    server =  start_webserver();

}

static int save_params(char *buff)
{
    nvs_handle_t my_handle;
    if (nvs_open("storage", NVS_READWRITE, &my_handle) != ESP_OK)
        return -1;

    char *token;
    token = strtok(buff, "&");
    while (token != NULL) {
        if (strstr(token, "ssid="))
            if (nvs_set_str(my_handle, "wifi_ssid", token + strlen("ssid=")) != ESP_OK)
                return -1;
        if (strstr(token, "pass="))
            if (nvs_set_str(my_handle, "wifi_password", token + strlen("pass=")) != ESP_OK)
                return -1;

        token = strtok(NULL, "&");
    }

    if (nvs_commit(my_handle) != ESP_OK)
        return -1;

    nvs_close(my_handle);
    return 0;
}