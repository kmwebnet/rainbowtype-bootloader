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

#include "wifitask.h"
#include "esp_wifi.h"
#include "lwip/api.h"
#include "esp_system.h"
#include "nvs_flash.h"

//wifi provisioning includes
#include "wifimgr/http_server.h"
#include "wifimgr/wifi_manager.h"
#include "driver/gpio.h"

#include "esp_tls.h"

ATCAIfaceCfg cfg;

bool connected = false;
char ssidresult[MAX_SSID_SIZE];
char passresult[MAX_PASSWORD_SIZE];
static TaskHandle_t task_http_server = NULL;
static TaskHandle_t task_wifi_manager = NULL;

extern uint8_t iokeyrandom[32];

static void savecredentials (char * wifissid, char * wifipass)
{

    atca_mbedtls_ecdh_ioprot_cb(iokeyrandom);
    uint8_t num_in[NONCE_NUMIN_SIZE] = { 0 };

    if (ATCA_SUCCESS != atcab_write_enc(8 , 0 , (uint8_t *)wifissid ,iokeyrandom ,  6 , num_in)) {
   	printf("read ssid to slot8 failed");
    }

    if (ATCA_SUCCESS != atcab_write_enc(8 , 1 , (uint8_t *)wifipass , iokeyrandom ,  6 , num_in)) {
   	printf("read pass1 to slot8 failed");
    }

    if (ATCA_SUCCESS != atcab_write_enc(8 , 2 , (uint8_t *)&wifipass[32] , iokeyrandom ,  6 , num_in)) {
   	printf("read pass2 to slot8 failed");
    }

return;
}

static void restorecredentials (char * wifissid, char * wifipass)
{

    atca_mbedtls_ecdh_ioprot_cb(iokeyrandom);
    uint8_t num_in[NONCE_NUMIN_SIZE] = { 0 };

    if (ATCA_SUCCESS != atcab_read_enc(8 , 0 , (uint8_t *)wifissid ,iokeyrandom ,  6 , num_in)) {
   	printf("read ssid to slot8 failed");
    }

    if (ATCA_SUCCESS != atcab_read_enc(8 , 1 , (uint8_t *)wifipass , iokeyrandom ,  6 , num_in)) {
   	printf("read pass1 to slot8 failed");
    }

    if (ATCA_SUCCESS != atcab_read_enc(8 , 2 , (uint8_t *)&wifipass[32] , iokeyrandom ,  6 , num_in)) {
   	printf("read pass2 to slot8 failed");
    }

return;
}



void wifiinit(char * wifissid, char * wifipass)
{

    get_atecc608cfg(&cfg);
    ATCA_STATUS status = atcab_init(&cfg);

    if (status != ATCA_SUCCESS) {
        printf("atcab_init() failed with ret=0x%08d\r\n", status);
        esp_restart();
    }
	

  restorecredentials(wifissid,  wifipass);

//wifi provisioning detection

	// configure button and led pins as GPIO pins
  PIN_FUNC_SELECT( IO_MUX_GPIO13_REG, PIN_FUNC_GPIO);

	gpio_pad_select_gpio(GPIO_NUM_4);
	gpio_pad_select_gpio(GPIO_NUM_13);
	
	// set the correct direction
	gpio_set_direction(GPIO_NUM_4, GPIO_MODE_INPUT);
  gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);
  gpio_set_pull_mode(GPIO_NUM_4, GPIO_PULLUP_ONLY);

  //LED off
  gpio_set_level(GPIO_NUM_13, 0);

	vTaskDelay(10 / portTICK_PERIOD_MS);

  if (0 == gpio_get_level(GPIO_NUM_4))
  {
  //indicates Wifi provisioning
    gpio_set_level(GPIO_NUM_13, 1);

	/* initialize flash memory */
	nvs_flash_init();

	/* start the HTTP Server task */
	xTaskCreate(&http_server, "http_server", 2048, NULL, 5, &task_http_server);

	/* start the wifi manager task */
	xTaskCreate(&wifi_manager, "wifi_manager", 4096, NULL, 4, &task_wifi_manager);

	while(1)
	{
		vTaskDelay(10 / portTICK_PERIOD_MS);

		if (connected)
		{
			break;
		}
	}

	printf("\nconnected\n");

	vTaskDelay(50 / portTICK_PERIOD_MS);

	vTaskDelete(task_http_server);
	tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP);
	vTaskDelay(50 / portTICK_PERIOD_MS);
	vTaskDelete(task_wifi_manager);

    esp_wifi_disconnect();
  	vTaskDelay(50 / portTICK_PERIOD_MS);
    esp_wifi_stop();
  	vTaskDelay(50 / portTICK_PERIOD_MS);
    esp_wifi_deinit();
    wifi_manager_destroy();

    savecredentials(ssidresult, passresult);

    esp_restart();


  }
	
return;
}