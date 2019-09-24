#ifndef _ALX_H_
#define _ALX_H_

enum lc_mode {CONSTANT, FADE_UP, FADE_DOWN, LIGHTS_OFF, LIGHTS_POWER_DOWN, LIGHTS_POWER_UP};

typedef struct {
	lc_mode mode;
	int brightness;
	int scheduled_color;
	int on_off_switch;
} lc_state_t;

typedef struct {
	lc_mode set_mode;
	int color;
	int set_bright;
	int max_bright;
	int min_bright;
	int refresh_delay;
} lc_config_t;

#endif /* _ALX_H_ */
