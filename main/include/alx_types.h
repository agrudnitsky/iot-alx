#ifndef _ALX_TYPES_H
#define _ALX_TYPES_H

#include <FastLED.h>
#include "local_settings.h"

enum lc_mode {CONSTANT, FADE_UP, FADE_DOWN, LIGHTS_OFF, LIGHTS_POWER_DOWN, LIGHTS_POWER_UP, TIME_DEPENDENT_COLORS, XMAS, LC_LAST_MODE};

#define LOGTAG_REST "rest"

typedef struct {
	lc_mode mode;
	int brightness;
	int scheduled_color;
	int color_palette;
	int on_off_switch;
	int remote_onoff;
	int use_transient_color;
	CRGB transient_color;
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
	int use_transient_color;
	CRGB transient_color;
} lc_config_t;

typedef struct {
	u_char* led_state;
	int pos;
	int num_leds;
	int state;
	int tick;
	int reps;
	int preparing;
	CRGB color;
} ledq_t;


extern CRGB sleds_0[NUM_LEDS];
extern CRGB sleds_1[NUM_LEDS];

extern CRGB leds_0[NUM_LEDS];
extern CRGB leds_1[NUM_LEDS];
extern ledq_t ledq;

extern lc_config_t lc_config;
extern CRGB color_palette[][6];
extern int color_palette_size[];


/* forward declarations */
uint32_t rand_bounded(unsigned int);
void flying_lights(ledq_t *);

void nvs_update_config(const char *nvs_namespace, const char *key, int val);
void nvs_update_coldef(const char *nvs_namespace, int color_palette, int color_id);

#endif
