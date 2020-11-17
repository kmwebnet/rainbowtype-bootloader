#include "ota.h"

#define HTTPS "https://"
#define STRING(str) #str
#define STR(str) STRING(str)

char httpsurl[100];

static const char *TAG = "ota";

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

void ota_start (void)
{

    strcpy(httpsurl , HTTPS);
    strcat(httpsurl , STR(CONFIG_WEBSOCKET_URI));
    strcat(httpsurl , "/fw/");
    strcat(httpsurl , serialstr);

    printf("http client setup for %s\n", httpsurl);

    esp_http_client_config_t config = {
        .url = httpsurl,
        .cert_pem = (const char *)rootca_crt_start,
        .event_handler = _http_event_handler,
    };

    esp_err_t ret = esp_https_ota(&config);
    if (ret == ESP_OK) {
      printf("success.and will reboot\n\n");
      fflush(stdout);
      esp_restart();
    } else {
      ESP_LOGE(TAG, "Firmware Upgrades Failed");
      esp_restart();
    }
}