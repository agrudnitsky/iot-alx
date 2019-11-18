#ifndef _ALX_TYPES_H
#define _ALX_TYPES_H

enum lc_mode {CONSTANT, FADE_UP, FADE_DOWN, LIGHTS_OFF, LIGHTS_POWER_DOWN, LIGHTS_POWER_UP};

#define LOGTAG_REST "rest"

typedef struct {
	lc_mode mode;
	int brightness;
	int scheduled_color;
	int color_palette;
	int on_off_switch;
	int remote_onoff;
} lc_state_t;

typedef struct {
	lc_mode set_mode;
	int color;
	int color_palette;
	int set_bright;
	int max_bright;
	int min_bright;
	int refresh_delay;
	int remote_onoff;
} lc_config_t;


/* forward declarations */
void nvs_update_config(const char *nvs_namespace, const char *key, int val);
void nvs_update_coldef(const char *nvs_namespace, int color_palette, int color_id);

#endif
