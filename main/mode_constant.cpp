#include "mode_constant.h"

void Mode_Constant::set_all_leds(CRGB color) {
	for(int i = 0; i < NUM_LEDS; i++) {
		leds_0[i] = color;
		leds_1[i] = color;
	}
}

void Mode_Constant::run(ledq_t *q, lc_config_t *lc_config) {
	set_all_leds(color_palette[lc_config->color_palette][lc_config->color]);
}
