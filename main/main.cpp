#include "alx.h"
#include "alx_types.h"
#include "local_settings.h"


xQueueHandle timer_queue;

CRGB leds[NUM_LEDS];

CRGB color_palette[][6] = {
	{CRGB::OrangeRed, CRGB::FloralWhite, CRGB::DeepPink,
	 CRGB::SpringGreen, CRGB::MediumBlue, CRGB::Indigo},
	{CRGB::OrangeRed, CRGB::FloralWhite, CRGB::FloralWhite, CRGB::FloralWhite, CRGB::FloralWhite, CRGB::OrangeRed}
};
int num_color_palettes = sizeof(color_palette)/sizeof(color_palette[0]);
int color_palette_size[sizeof(color_palette)/sizeof(color_palette[0])];

lc_state_t lc_state;
lc_config_t lc_config;

static EventGroupHandle_t s_wifi_event_group;
static int WIFI_CONNECTED_BIT = BIT0;
static int s_retry_num = 0;
static int net_startup = 1;
static int schedule_netup_actions = 0;

const char *version = VERSION;

void (*mode_helper_fun[LC_LAST_MODE])();

esp_http_client_config_t http_client_conf = {
	.url = "http://www.google.com/",
	.event_handler = _http_header_to_datetime,
};

/* format: secs past midnight, palette, color_id, brightness */
tdc_entry_t time_colors[6] = {
	{( 0*60 + 30)*60, 1, 0,  90},
	{( 5*60 +  0)*60, 1, 1,  90},
	{( 8*60 +  0)*60, 1, 2, 200},
	{(12*60 + 30)*60, 1, 3, 240},
	{(16*60 + 30)*60, 1, 4, 240},
	{(21*60 + 30)*60, 1, 5, 140}
};
int num_time_colors = sizeof(time_colors)/sizeof(time_colors[0]);


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
		schedule_netup_actions = 1;
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
			if (lc_config.set_bright <= ++lc_state.brightness) lc_state.mode = lc_config.set_mode;
			break;
		case CONSTANT:
			lc_config.use_transient_color = 0;
		case TIME_DEPENDENT_COLORS:
		default:
			lc_state.brightness = lc_config.set_bright;
			lc_state.scheduled_color = lc_config.color;
			lc_state.color_palette = lc_config.color_palette;
			lc_state.mode = lc_config.set_mode;
			break;
		};

		/* should never happen */
		if (lc_state.brightness < 0) lc_state.brightness = lc_config.set_bright;

		FastLED.setBrightness(lc_state.brightness);
		if (lc_state.use_transient_color) {
			refresh_leds(lc_state.transient_color);
		} else {
			refresh_leds(color_palette[lc_state.color_palette][lc_state.scheduled_color]);
		}

		on_off_switch = gpio_get_level(GPIO_NUM_32);
		if ((!on_off_switch && lc_state.on_off_switch)
		    || (!lc_config.remote_onoff && lc_state.remote_onoff)) {
			/* 1 -> 0 */
			lc_state.mode = LIGHTS_POWER_DOWN;
			ESP_LOGI(LOGTAG_LC, "lights off");
		} else if ((on_off_switch && !lc_state.on_off_switch)
			   || (lc_config.remote_onoff && !lc_state.remote_onoff)) {
			/* 0 -> 1 */
			lc_state.mode = LIGHTS_POWER_UP;
			ESP_LOGI(LOGTAG_LC, "lights on");
		}
		if (on_off_switch != lc_state.on_off_switch) {
			lc_state.on_off_switch = on_off_switch;
			/* physical switch action overrides remote on/off */
			lc_state.remote_onoff = on_off_switch;
			lc_config.remote_onoff = on_off_switch;
		}
		lc_state.remote_onoff = lc_config.remote_onoff;
	}
}


void tdc_color_lookup() {
	int i;
	struct timeval now_te;
	time_t now_ts;
	struct tm *now;
	int secs_past_mn;
	int next_palette = lc_config.color_palette;
	int next_color = lc_config.color;
	int next_bright = lc_config.set_bright;
	int last_tdc_id = -1;
	int next_tdc_id = -1;
	int tdc_distance = 0;
	int secs_past_last_tdc;
	int time_progress;

	gettimeofday(&now_te, NULL);
	now_ts = now_te.tv_sec;
	now = localtime(&now_ts);
	secs_past_mn = now->tm_hour*3600 + now->tm_min*60 + now->tm_sec;
	ESP_LOGI(LOGTAG_MISC, "now: %d:%02d:%02d", now->tm_hour, now->tm_min, now->tm_sec);

	for (i = 0; i < num_time_colors; ++i) {
		if (secs_past_mn > time_colors[i].secs_past_mn) {
			last_tdc_id = i;
		}
	}
	if (-1 == last_tdc_id) last_tdc_id = num_time_colors-1;

	ESP_LOGI(LOGTAG_MISC, "last_tdc_id = %d, num_time_colors = %d", last_tdc_id, num_time_colors);
	if (last_tdc_id == num_time_colors-1) {
		next_tdc_id = 0;
		tdc_distance = 24*60*60;
	} else {
		next_tdc_id = last_tdc_id + 1;
	}
	tdc_distance += time_colors[next_tdc_id].secs_past_mn - time_colors[last_tdc_id].secs_past_mn;

	secs_past_last_tdc = secs_past_mn - time_colors[last_tdc_id].secs_past_mn;
	if (secs_past_mn < time_colors[last_tdc_id].secs_past_mn) secs_past_last_tdc += 24*60*60;
	time_progress = (100*secs_past_last_tdc) / tdc_distance;

	/* interpolate brightness */
	lc_config.set_bright = time_colors[last_tdc_id].brightness + time_progress*(time_colors[next_tdc_id].brightness - time_colors[last_tdc_id].brightness)/100;

	lc_config.color_palette = time_colors[last_tdc_id].palette;
	lc_config.color = time_colors[last_tdc_id].color;

	/* interpolate color */
	/* XXX: not sure if we should do this in HSV
	 * this would probably require all LED operations to be done in HSV, though
	 */
	lc_state.use_transient_color = 1;
	lc_state.transient_color.red = color_palette[time_colors[last_tdc_id].palette][time_colors[last_tdc_id].color].red + time_progress*(color_palette[time_colors[next_tdc_id].palette][time_colors[next_tdc_id].color].red - color_palette[time_colors[last_tdc_id].palette][time_colors[last_tdc_id].color].red)/100;
	lc_state.transient_color.green = color_palette[time_colors[last_tdc_id].palette][time_colors[last_tdc_id].color].green + time_progress*(color_palette[time_colors[next_tdc_id].palette][time_colors[next_tdc_id].color].green - color_palette[time_colors[last_tdc_id].palette][time_colors[last_tdc_id].color].green)/100;
	lc_state.transient_color.blue = color_palette[time_colors[last_tdc_id].palette][time_colors[last_tdc_id].color].blue + time_progress*(color_palette[time_colors[next_tdc_id].palette][time_colors[next_tdc_id].color].blue - color_palette[time_colors[last_tdc_id].palette][time_colors[last_tdc_id].color].blue)/100;

	ESP_LOGI(LOGTAG_LC, " palette: %d, color: %d, bright: %d (last: %d, next: %d), time_progress: %d (secs_past last_tdc: %d, tdc_distance: %d)", lc_config.color_palette, lc_config.color, lc_config.set_bright, time_colors[last_tdc_id].brightness, time_colors[next_tdc_id].brightness, time_progress, secs_past_last_tdc, tdc_distance);
	ESP_LOGI(LOGTAG_LC, " color: (%d, %d, %d), transient(%d): (%d, %d, %d)", color_palette[lc_config.color_palette][lc_config.color].red, color_palette[lc_config.color_palette][lc_config.color].green, color_palette[lc_config.color_palette][lc_config.color].blue, lc_state.use_transient_color, lc_state.transient_color.red, lc_state.transient_color.green, lc_state.transient_color.blue);
}


esp_err_t _http_header_to_datetime(esp_http_client_event_t *ev) {
	struct tm htime;
	time_t now;
	struct timeval tv_now;

	switch (ev->event_id) {
	case HTTP_EVENT_ON_HEADER:
		if (0 == strncmp("Date", ev->header_key, 4)) {
			strptime((char *)ev->header_value, "%a, %d %b %Y %T", &htime);
			ESP_LOGI(LOGTAG_MISC, "Datetime from HTTP: %s", ev->header_value);
			now = mktime(&htime);
			tv_now.tv_sec = now;
			/* time from HTTP header should be in GMT,
			 * thus we set the correct timezone after setting time */
			settimeofday(&tv_now, NULL);

			/* set timezone */
			setenv("TZ", TZ_SPEC, 1);
			tzset();
		}
		break;
	case HTTP_EVENT_ERROR:
	case HTTP_EVENT_ON_CONNECTED:
	case HTTP_EVENT_HEADERS_SENT:
	case HTTP_EVENT_ON_DATA:
	case HTTP_EVENT_ON_FINISH:
	case HTTP_EVENT_DISCONNECTED:
		break;
	}

	return ESP_OK;
}


void netup_actions() {
	/* get date/time from HTTP response header */
	esp_http_client_handle_t http_client = esp_http_client_init(&http_client_conf);
	esp_err_t err = esp_http_client_perform(http_client);
	esp_http_client_cleanup(http_client);
}


void house_keeper(void *arg) {
	timer_event_t ev;
	wifi_ap_record_t ap_info;

	while (1) {
		xQueueReceive(timer_queue, &ev, portMAX_DELAY);
		if (ESP_ERR_WIFI_NOT_CONNECT == esp_wifi_sta_get_ap_info(&ap_info)) {
			reinit_net();
		} else if (schedule_netup_actions) {
			netup_actions();
			schedule_netup_actions = 0;
		}

		if (NULL != mode_helper_fun[lc_state.mode]) {
			mode_helper_fun[lc_state.mode]();
		}

	}
}


void init_lc(lc_state_t *lcs, lc_config_t *lcc) {
	int cs, i;
	nvs_handle_t nvsh_load;
	char key[11];
	uint32_t colval;

	/* mode helper functions */
	for (i = 0; i < LC_LAST_MODE; ++i) mode_helper_fun[i] = NULL;
	mode_helper_fun[TIME_DEPENDENT_COLORS] = tdc_color_lookup;

	/* color palettes */
	for (i = 0; i < num_color_palettes; ++i) {
		color_palette_size[i] = sizeof(color_palette[i])/sizeof(color_palette[i][0]);
	}

	/* LC state / LC config defaults */
	lcs->brightness = 0;
	lcs->scheduled_color = 0;
	lcs->color_palette = 0;
	lcs->on_off_switch = gpio_get_level(GPIO_NUM_32);
	lcs->remote_onoff = lcs->on_off_switch;
	lcs->mode = lcs->on_off_switch ? CONSTANT : LIGHTS_OFF;

	lcc->set_bright = 90;
	lcc->color = 0;
	lcc->color_palette = 0;
	lcc->refresh_delay = 20;
	lcc->max_bright = 254;
	lcc->min_bright = 0;
	lcc->set_mode = CONSTANT;
	lcc->remote_onoff = lcs->on_off_switch;

	/* load config from NVS */
	esp_err_t err = nvs_open("alx.lcc", NVS_READONLY, &nvsh_load);
	if (ESP_OK != err) {
		ESP_LOGW(LOGTAG_MISC, "failed opening nvs for loading");
		return;
	}
	/* config */
	nvs_get_i32(nvsh_load, "mode", (int *)&(lcc->set_mode));
	nvs_get_i32(nvsh_load, "set_bright", &(lcc->set_bright));
	nvs_get_i32(nvsh_load, "color", &(lcc->color));
	nvs_get_i32(nvsh_load, "color_palette", &(lcc->color_palette));

	/* color definitions */
	for (cs = 0; cs < num_color_palettes; ++cs) {
		for (i = 0; i < color_palette_size[cs]; ++i) {
			sprintf(key, "col%03u.%03u", cs, i);
			if (ESP_OK == nvs_get_u32(nvsh_load, key, &colval)) {
				color_palette[cs][i].r = (0x00FF0000 & colval) >> 16;
				color_palette[cs][i].g = (0x0000FF00 & colval) >> 8;
				color_palette[cs][i].b = 0x000000FF & colval;
			}
		}
	}

	nvs_close(nvsh_load);
}


void nvs_update_config(const char *nvs_namespace, const char *key, int val) {
	nvs_handle_t nvsh_update;
	nvs_open(nvs_namespace, NVS_READWRITE, &nvsh_update);
	nvs_set_i32(nvsh_update, key, val);
	nvs_commit(nvsh_update);
	nvs_close(nvsh_update);
}


void nvs_update_coldef(const char *nvs_namespace, int cs, int color_id) {
	uint32_t storeval = (color_palette[cs][color_id].r << 16) | (color_palette[cs][color_id].g << 8) | color_palette[cs][color_id].b;
	nvs_handle_t nvsh_update;
	char key[11];

	sprintf(key, "col%03u.%03u", cs, color_id);

	nvs_open(nvs_namespace, NVS_READWRITE, &nvsh_update);

	nvs_set_u32(nvsh_update, key, storeval);
	nvs_commit(nvsh_update);
	nvs_close(nvsh_update);
}


void nvs_lc_init() {
	/* write default values to nvs */
	nvs_handle_t nvsh_write;
	ESP_ERROR_CHECK(nvs_open("alx.lcc", NVS_READWRITE, &nvsh_write));
	nvs_set_i32(nvsh_write, "set_bright", 90);
	nvs_set_i32(nvsh_write, "color", 0);
	nvs_set_i32(nvsh_write, "color_palette", 0);
	nvs_set_i32(nvsh_write, "set_mode", CONSTANT);
	nvs_commit(nvsh_write);
	nvs_close(nvsh_write);
}


void init_io() {
	gpio_set_direction(GPIO_NUM_32, GPIO_MODE_INPUT);
}


void reinit_net() {
	/* TODO: This doesn't seem to work
	 * -> force reinit for debugging purposes a few seconds after start
	 * and observe what happens
	 */
	ESP_LOGI(LOGTAG_WIFI, "wifi reinit.");

	ESP_ERROR_CHECK(esp_wifi_stop());
	s_retry_num = 0;
	tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, MYHOSTNAME);
	esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler);
	esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler);
	esp_wifi_deinit();
	vEventGroupDelete(s_wifi_event_group);

	init_net();
}


void init_net() {
	wifi_config_t wifi_config;


	/* Init WIFI */
	s_wifi_event_group = xEventGroupCreate();

	if (net_startup) {
		tcpip_adapter_init();
		ESP_ERROR_CHECK(esp_event_loop_create_default());
	}


	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	cfg.nvs_enable = 0;

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

	net_startup = 0;

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
		nvs_lc_init();
	}
	ESP_ERROR_CHECK(ret);

	init_io();
	init_net();
	/* XXX: for some reason we have to init wifi
	 * before we screw around with NVS,
	 * even with nvs_enabled = 0 in wifi config
	 */
	init_lc(&lc_state, &lc_config);
	init_fs();
	init_timers();

	/* REST server - Core 0 */
	start_rest_server(WEB_MOUNT_POINT, 0);

	/* House Keeping - Core 0 */
	xTaskCreatePinnedToCore(&house_keeper, "house_keeper", 2048, NULL, 4, NULL, 0);

	/* Light Controller - Core 1 */
	xTaskCreatePinnedToCore(&room_lights, "room_lights", 4000, NULL, 5, NULL, 1);
}
