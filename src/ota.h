#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

extern const uint8_t rootca_crt_start[] asm("_binary_rootca_crt_start");
extern const uint8_t rootca_crt_end[] asm("_binary_rootca_crt_end");

extern char serialstr[20];

extern char url[100];

esp_err_t _http_event_handler(esp_http_client_event_t *evt);

void ota_start (void);
