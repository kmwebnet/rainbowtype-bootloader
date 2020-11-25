/*
MIT License

Copyright (c) 2020 kmwebnet

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "tcpip_adapter.h"
#include "esp_event.h"

#include "esp_log.h"

#include "driver/gpio.h"

#include "nvs_flash.h"

#include "esp_ota_ops.h"

#include "esp_tls.h"
#include "cryptoauthlib.h"

#include "wifitask.h"
#include "configure.h"
#include "ota.h"


#define WSS "wss://"
#define STRING(str) #str
#define STR(str) STRING(str)

char wifissid[32];
char wifipass[64];
char url[100] = {};
char serialstr[20] = {};
uint8_t configbytes[32] ={};

bool noneresponse   = false;
bool otastart       = false;
bool getnextkey     = false;
bool certificate    = false;
bool geturl         = false;
bool rebootflag     = false;
uint8_t keynum = 0;
uint8_t nextkey = 0;
char pubkeypem[200];
char newdevicecert[1000] = {};
char recieveurl[400] = {};

statetype currentstate;

#include "esp_websocket_client.h"

static const char *TAG = "WEBSOCKET";

esp_websocket_client_handle_t client;

extern const uint8_t rootca_crt_start[] asm("_binary_rootca_crt_start");
extern const uint8_t rootca_crt_end[] asm("_binary_rootca_crt_end");

static EventGroupHandle_t wifi_event_group;

#ifndef BIT0
#define BIT0 (0x1 << 0)
#endif

const int CONNECTED_BIT = BIT0;

ATCAIfaceCfg cfg;




static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP platform WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}


static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {};
    strcpy ((char *)wifi_config.sta.ssid, wifissid);
    strcpy ((char *)wifi_config.sta.password, wifipass); 
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void actiontask(char * datacontents)
{

    char* pos;
    char* npos;

if (NULL == strstr(datacontents, serialstr))
{
    return;
}

// ignore rtClient action
if (NULL != strstr(datacontents, "setstate"))
{
    goto exit;

}

// device state change

    if (NULL != (pos = strstr(datacontents, "state")))
    {
        pos += 6;
        npos = strstr(pos, "\""); 
        npos ++;

        if (NULL != strstr(npos, "offlineboot"))
        {
          currentstate = offlineboot;
          setcurrentstate(currentstate);
          printf("set state to offlineboot\n");
        }  

        if (NULL != strstr(npos, "onlineboot"))
        {
          currentstate = onlineboot;
          setcurrentstate(currentstate);
          printf("set state to onlineboot\n");        
        }  

        if (NULL != strstr(npos, "forcefwdownload"))
        {
          currentstate = forcefwdownload;
          setcurrentstate(currentstate);
          printf("set state to forcefwdownload\n");          
        }  

        if (NULL != strstr(npos, "bootprohibited"))
        {
          currentstate = bootprohibited;
          setcurrentstate(currentstate);
          printf("set state to bootprohobited\n");         
        }  

    }

// server request response decode

    if (NULL != (pos = strstr(datacontents, "request")))
    {
        pos += 8;
        npos = strstr(pos, "\""); 
        npos ++;

        char extstr[10];
        strncpy(extstr, npos, 10);
        extstr[10] = '\0';  
        

        if (NULL != strstr(extstr, "none"))
        {
          noneresponse = true;
          goto exit;
        }    

        if (NULL != strstr(extstr, "ota"))
        {
          otastart = true;
          goto exit;
        }

        if (NULL != strstr(extstr, "url"))
        {
          pos = strstr(datacontents, "data");
          if (pos == NULL)
          {
            printf("data required. data format error");
            goto exit;       
          }


          pos += 7;
          npos = strstr(pos, "\"");

          if ((npos - pos) > 255)
          {
            printf("data too large. data format error");
            goto exit;       
          }

          strncpy(recieveurl, pos, (npos - pos));

          geturl = true;
          rebootflag = true;
          goto exit;
        }

        if (NULL != strstr(extstr, "pubkey"))
        {
          printf("pubkey request\n");
          getnextkey = true;
          rebootflag = true;
          goto exit;
        }

        if (NULL != strstr(extstr, "cert") && !certificate)
        {

          pos = strstr(datacontents, "keynum");
          if (pos == NULL)
          {
            printf("keynum required. Cert format error");
            goto exit;       
          }
          pos += 8;
          npos = pos;
          //npos ++;
          nextkey = (uint8_t)*npos - 48;
          printf("keynum:%d\n", nextkey);

          ATCA_STATUS status = pickpemcert(datacontents, newdevicecert);
      
          if (status != ATCA_SUCCESS)
          {
            printf("Cert format error");
            goto exit;
          }

          certificate = true;
          rebootflag = true;
          goto exit;
        }
    }
    exit:
    memset (datacontents, 0, 20);
    return;

}


static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id) {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        if (!rebootflag) {
            char pubMessage[128];
            sprintf(pubMessage, 
            "{\"serial\": \"%s\", "
	        "\"action\": \"bootup\"}",
            serialstr) ;
            esp_websocket_client_send(client, pubMessage, strlen(pubMessage), portMAX_DELAY);
        }
        else
        {
            rebootflag = false;
        }
        
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        break;
    case WEBSOCKET_EVENT_DATA:
        //ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA");
        //ESP_LOGI(TAG, "Received opcode=%d", data->op_code);
        ESP_LOGW(TAG, "Received=%.*s", data->data_len, (char *)data->data_ptr);
        //ESP_LOGW(TAG, "Total payload length=%d, data_len=%d, current payload offset=%d\r\n", data->payload_len, data->data_len, data->payload_offset);
        actiontask((char *)data->data_ptr);
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_ERROR");
        break;
    }
}

static void stateupdate(void)
{

  currentstate = getcurrentstate();
  printf("currrent state: %s\n",statestr[currentstate]); 

  if (currentstate == offlineboot )
    {
      esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL));
    }

}

static void websocket_app_start(void)
{
    get_atecc608cfg(&cfg);
    ATCA_STATUS status = atcab_init(&cfg);

    if (status != ATCA_SUCCESS) {
        ESP_LOGE(TAG, "atcab_init() failed with ret=0x%08d\r\n", status);
        esp_restart();
    }

    uint8_t serial[ATCA_SERIAL_NUM_SIZE];
    status = atcab_read_serial_number(serial);

    if (status != ATCA_SUCCESS) {
	    ESP_LOGE(TAG, "atcab_read_serial_number() failed with ret=0x%08d/r/n", status);
        esp_restart();
    }

    char character[3] = {};
    for ( int i = 0; i< 9; i++)
        {
        sprintf(character , "%02x", serial[i]);
	    strcat(serialstr, character);
        }

    keynum = sysinit();

	// configure button and led pins as GPIO pins
    PIN_FUNC_SELECT( IO_MUX_GPIO13_REG, PIN_FUNC_GPIO);

	gpio_pad_select_gpio(GPIO_NUM_13);
	
	// set the correct direction
    gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);
	




    esp_websocket_client_config_t websocket_cfg = {};

    strcpy(url, WSS);
    strcat(url ,STR(CONFIG_WEBSOCKET_URI));
    strcat(url ,"/rtserver");
    websocket_cfg.uri = url;
    websocket_cfg.cert_pem = (const char *)rootca_crt_start; 
    websocket_cfg.task_stack = (8*1024);

    client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);

    esp_websocket_client_start(client);

    char pkeyMessage[1000];
    int led = 0;

    while(1)
    {

        if (noneresponse)
        {

          sprintf(pkeyMessage, 
          "{\"serial\": \"%s\", "
          "\"action\": \"updatecompleted\"}",
          serialstr) ;
          esp_websocket_client_send(client, pkeyMessage, strlen(pkeyMessage), portMAX_DELAY);

          noneresponse = false;

          vTaskDelay(100 / portTICK_PERIOD_MS);
          break;

        }

        if (getnextkey)
        {
          gpio_set_level(GPIO_NUM_13, 1);

          esp_websocket_client_stop(client);
          getpubkey(&nextkey, pubkeypem);
          replace_chr(pubkeypem , '\r', "" );
          replace_chr(pubkeypem , '\n', "\\n");

          printf("next key slot is:%d\n", nextkey);

          esp_websocket_client_start(client);

          sprintf(pkeyMessage, 
          "{\"serial\": \"%s\", "
          "\"keynum\": %d, "  
          "\"data\": \"%s\", "  
          "\"action\": \"publickeyissued\"}",
          serialstr, nextkey, pubkeypem) ;
          esp_websocket_client_send(client, pkeyMessage, strlen(pkeyMessage), portMAX_DELAY);

          getnextkey = false;

          vTaskDelay(100 / portTICK_PERIOD_MS);
          break;

        }

        if (certificate )
        {
          gpio_set_level(GPIO_NUM_13, 1);
          
          esp_websocket_client_stop(client);

          replace_str_all(newdevicecert , "\\\\n", "\n" );
          printf("prepare certificate setting\n");
          printf("\n");

          fflush(stdout);

          ATCA_STATUS status = setdevicecert(newdevicecert, nextkey);
          if(status != ATCA_SUCCESS)
          {
            printf("failed to set next device cert%d\n", status);
          }

          keynum = nextkey;

          esp_websocket_client_start(client);


          sprintf(pkeyMessage, 
          "{\"serial\": \"%s\", "
          "\"action\": \"finishcertrefresh\"}",
          serialstr) ;
          esp_websocket_client_send(client, pkeyMessage, strlen(pkeyMessage), portMAX_DELAY);
 
          certificate = false;

          vTaskDelay(100 / portTICK_PERIOD_MS);
          break;

        }

        if (geturl)
        {

          esp_websocket_client_stop(client);

          savemiscdata((uint8_t *)recieveurl);

          char tmpdata[256];
          restoremiscdata((uint8_t * )tmpdata);

          esp_websocket_client_start(client);

          sprintf(pkeyMessage, 
          "{\"serial\": \"%s\", "
          "\"action\": \"urlupdated\"}",
          serialstr) ;
          esp_websocket_client_send(client, pkeyMessage, strlen(pkeyMessage), portMAX_DELAY);

          geturl = false;

          vTaskDelay(100 / portTICK_PERIOD_MS);
          break;

        }


        if (otastart)
        {

          sprintf(pkeyMessage, 
          "{\"serial\": \"%s\", "
          "\"action\": \"otaaccepted\"}",
          serialstr) ;
          esp_websocket_client_send(client, pkeyMessage, strlen(pkeyMessage), portMAX_DELAY);

          vTaskDelay(100 / portTICK_PERIOD_MS);
          break;
        }

    led++;
    gpio_set_level(GPIO_NUM_13, led % 2);

    vTaskDelay(100 / portTICK_PERIOD_MS);

    }

    gpio_set_level(GPIO_NUM_13, 0);

    esp_websocket_client_stop(client);
    ESP_LOGI(TAG, "Websocket Stopped");
    esp_websocket_client_destroy(client);

    if (otastart || currentstate == forcefwdownload)
    {
      ota_start();
    }

    if (currentstate == offlineboot || currentstate == onlineboot)
    {
      esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL));
      esp_restart();
    }

    while(1)
    {
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }

}



void app_main()
{
    wifiinit(wifissid, wifipass);
    // Initialize NVS
    stateupdate();
    ESP_ERROR_CHECK(nvs_flash_init());
    initialise_wifi();
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,false, true, portMAX_DELAY);
    websocket_app_start();
}