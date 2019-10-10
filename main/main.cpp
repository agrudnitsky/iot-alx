#include "alx.h"
#include "alx_types.h"
#include "local_settings.h"


xQueueHandle timer_queue;

CRGB leds[NUM_LEDS];

CRGB color_schedule[] = {
	CRGB::OrangeRed, CRGB::FloralWhite, CRGB::DeepPink,
	CRGB::SpringGreen, CRGB::MediumBlue, CRGB::Indigo
};
int color_schedule_size = sizeof(color_schedule)/sizeof(color_schedule[0]);

lc_state_t lc_state;
lc_config_t lc_config;

static EventGroupHandle_t s_wifi_event_group;
static int WIFI_CONNECTED_BIT = BIT0;
static int s_retry_num = 0;

const char *version = VERSION;


extern "C" {
	void app_main();
}


static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	int conn_ret = 0;

	if (WIFI_EVENT == event_base) {
		ESP_LOGI(LOGTAG_WIFI, "wifi event: %d", event_id);
		if (WIFI_EVENT_STA_START == event_id) {
			conn_ret = esp_wifi_connect();
			ESP_LOGI(LOGTAG_WIFI, "conn_ret: %d", conn_ret);
		} else if (WIFI_EVENT_STA_DISCONNECTED == event_id) {
			if (s_retry_num < WIFI_MAXIMUM_RETRY) {
				conn_ret = esp_wifi_connect();
				ESP_LOGI(LOGTAG_WIFI, "conn_ret: %d", conn_ret);
				xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
				s_retry_num++;
				ESP_LOGI(LOGTAG_WIFI, "AP connection retry");
			}
			ESP_LOGI(LOGTAG_WIFI, "AP connection failed");
		}
	} else if (IP_EVENT == event_base && IP_EVENT_STA_GOT_IP == event_id) {
		ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
		ESP_LOGI(LOGTAG_WIFI, "got ip: %s", ip4addr_ntoa(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}


void IRAM_ATTR timer_group0_isr(void *par) {
	int timer_id = (int)par;
	timer_intr_t timer_intr = timer_group_intr_get_in_isr(TIMER_GROUP_0);
	uint64_t timer_counter_value = timer_group_get_counter_value_in_isr(TIMER_GROUP_0, (timer_idx_t)timer_id);

	timer_event_t ev;
	ev.group = TIMER_GROUP_0;
	ev.id = timer_id;
	ev.counter_value = timer_counter_value;

	if (timer_intr & TIMER_INTR_T0) {
		timer_group_intr_clr_in_isr(TIMER_GROUP_0, TIMER_0);
		timer_counter_value += (uint64_t)(HKT_INTERVAL * TIMER_SCALE);
		timer_group_set_alarm_value_in_isr(TIMER_GROUP_0, TIMER_0, timer_counter_value);
	}

	timer_group_enable_alarm_in_isr(TIMER_GROUP_0, (timer_idx_t)timer_id);

	xQueueSendFromISR(timer_queue, &ev, NULL);
}


void refresh_leds(CRGB color) {
	for(int i = 0; i < NUM_LEDS; i++) {
		leds[i] = color;
	}
	FastLED.show();
	FastLED.delay(lc_config.refresh_delay);
}


void room_lights(void *arg){
	int on_off_switch = 0;

	FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
	FastLED.setMaxPowerInVoltsAndMilliamps(5,3500);
	FastLED.setCorrection(TypicalSMD5050);

	while (1) {
		switch (lc_state.mode) {
		case LIGHTS_OFF:
			lc_state.brightness = 0;
			break;
		case LIGHTS_POWER_DOWN:
			if (1 >= (lc_state.brightness = dim8_lin(lc_state.brightness))) lc_state.mode = LIGHTS_OFF;
			break;
		case LIGHTS_POWER_UP:
			if (lc_config.set_bright <= ++lc_state.brightness) lc_state.mode = CONSTANT;
			break;
		case CONSTANT:
		default:
			lc_state.brightness = lc_config.set_bright;
			lc_state.scheduled_color = lc_config.color;
			break;
		};

		/* should never happen */
		if (lc_state.brightness < 0) lc_state.brightness = lc_config.set_bright;

		FastLED.setBrightness(lc_state.brightness);
		refresh_leds(color_schedule[lc_state.scheduled_color]);

		on_off_switch = gpio_get_level(GPIO_NUM_32);
		if (!on_off_switch && lc_state.on_off_switch) {
			/* 1 -> 0 */
			lc_state.mode = LIGHTS_POWER_DOWN;
			ESP_LOGI(LOGTAG_LC, "lights off");
		} else if (on_off_switch && !lc_state.on_off_switch) {
			/* 0 -> 1 */
			lc_state.mode = LIGHTS_POWER_UP;
			ESP_LOGI(LOGTAG_LC, "lights on");
		}
		lc_state.on_off_switch = on_off_switch;
	}
}


void house_keeper(void *arg) {
	timer_event_t ev;
	wifi_ap_record_t ap_info;

	while (1) {
		xQueueReceive(timer_queue, &ev, portMAX_DELAY);
		if (ESP_ERR_WIFI_NOT_CONNECT == esp_wifi_sta_get_ap_info(&ap_info)) {
			reinit_net();
		}
	}
}


void init_lc(lc_state_t *lcs, lc_config_t *lcc) {
	lcs->mode = CONSTANT;
	lcs->brightness = 0;
	lcs->scheduled_color = 0;
	lcs->on_off_switch = gpio_get_level(GPIO_NUM_32);

	lcc->refresh_delay = 20;
	lcc->color = 0;
	lcc->max_bright = 254;
	lcc->min_bright = 0;
	lcc->set_bright = 90;
	lcc->set_mode = CONSTANT;
}


void init_io() {
	gpio_set_direction(GPIO_NUM_32, GPIO_MODE_INPUT);
}


void reinit_net() {
	ESP_ERROR_CHECK(esp_wifi_stop());
	s_retry_num = 0;
	ESP_ERROR_CHECK(esp_wifi_start());
	tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, MYHOSTNAME);

	ESP_LOGI(LOGTAG_WIFI, "wifi reinit.");
}


void init_net() {
	wifi_config_t wifi_config;


	/* Init WIFI */
	s_wifi_event_group = xEventGroupCreate();

	tcpip_adapter_init();

	ESP_ERROR_CHECK(esp_event_loop_create_default());

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

	strcpy((char *)wifi_config.sta.ssid, WIFI_SSID);
	strcpy((char *)wifi_config.sta.password, WIFI_PASS);
	wifi_config.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
	wifi_config.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
	wifi_config.sta.threshold.rssi = -127;
	wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, MYHOSTNAME);

	ESP_LOGI(LOGTAG_WIFI, "init_net finished.");
}


esp_err_t init_fs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = WEB_MOUNT_POINT,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(LOGTAG_MISC, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(LOGTAG_MISC, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(LOGTAG_MISC, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(LOGTAG_MISC, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(LOGTAG_MISC, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}


void init_timers() {
	timer_config_t conf;

	timer_queue = xQueueCreate(10, sizeof(timer_event_t));
	conf.divider = TIMER_DIVIDER;
	conf.counter_dir = TIMER_COUNT_UP;
	conf.counter_en = TIMER_PAUSE;
	conf.alarm_en = TIMER_ALARM_EN;
	conf.intr_type = TIMER_INTR_LEVEL;
	conf.auto_reload = TIMER_AUTORELOAD_EN;

	timer_init(TIMER_GROUP_0, TIMER_0, &conf);
	timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
	timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, HKT_INTERVAL*TIMER_SCALE);
	timer_enable_intr(TIMER_GROUP_0, TIMER_0);
	timer_isr_register(TIMER_GROUP_0, TIMER_0, timer_group0_isr, (void *) TIMER_0, ESP_INTR_FLAG_IRAM, NULL);
	timer_start(TIMER_GROUP_0, TIMER_0);
}


void app_main() {
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	init_lc(&lc_state, &lc_config);
	init_io();
	init_net();
	init_fs();
	init_timers();

	/* REST server - Core 0 */
	start_rest_server(WEB_MOUNT_POINT, 0);

	/* House Keeping - Core 0 */
	xTaskCreatePinnedToCore(&house_keeper, "house_keeper", 2048, NULL, 4, NULL, 0);

	/* Light Controller - Core 1 */
	xTaskCreatePinnedToCore(&room_lights, "room_lights", 4000, NULL, 5, NULL, 1);
}
