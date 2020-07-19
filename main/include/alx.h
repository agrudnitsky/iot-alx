#ifndef _ALX_H_
#define _ALX_H_


#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>

#include <esp_system.h>
#include <esp_spi_flash.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_spiffs.h>
#include "esp_http_client.h"
#include <esp_netif.h>

#include <FastLED.h>

#include <driver/gpio.h>
#include <driver/adc.h>
#include <driver/timer.h>

#include <lwip/err.h>
#include <lwip/sys.h>

#include "local_settings.h"
#include "alx_types.h"


#define WIFI_MAXIMUM_RETRY 2
#define WEB_MOUNT_POINT "/www"
#define LOGTAG_WIFI "wifi"
#define LOGTAG_MISC "misc"
#define LOGTAG_LC "lc"
#define LOGTAG_HK "hk"

#define TIMER_DIVIDER 16
#define TIMER_SCALE (TIMER_BASE_CLK/TIMER_DIVIDER)
#define HKT_INTERVAL 5


typedef struct {
	timer_group_t group;
	int id;
	uint64_t counter_value;
} timer_event_t;

typedef struct {
	int secs_past_mn;
	int palette;
	int color;
	int brightness;
} tdc_entry_t;




/* forward declarations */
void reinit_net();
void init_net();
esp_err_t start_rest_server(const char *base_path, int core_id);
esp_err_t _http_header_to_datetime(esp_http_client_event_t *ev);


#endif /* _ALX_H_ */
