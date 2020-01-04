#include "alx_types.h"

void prepare_next_light(ledq_t *q) {
	lc_config.color = rand_bounded(color_palette_size[lc_config.color_palette]);
	leds_0[0] = color_palette[lc_config.color_palette][lc_config.color];
	leds_1[0] = color_palette[lc_config.color_palette][lc_config.color];
	q->led_state[0] = 1;
}

void flying_lights(ledq_t *q) {
	int i;
	CRGB tmp_led0;
	CRGB tmp_led1;

	switch (q->state) {
	case 0: /* prepare and flare up */
		if (0 == q->preparing) {
			prepare_next_light(q);
			q->preparing = 1;
		}
		if (q->preparing > 0) {
			/* flare up */
			if (0 == (q->preparing % 4)) {
				leds_0[0] = color_palette[lc_config.color_palette][lc_config.color];
				leds_1[0] = color_palette[lc_config.color_palette][lc_config.color];
				leds_0[0] /= 8-q->preparing/4;
				leds_1[0] /= 8-q->preparing/4;
			}

			q->preparing++;
			if (32 == q->preparing) {
				q->preparing = 0;
			}
		}
		/* fall through */
	case 2: /* flying */
		for (i = q->num_leds-1; i >= 0; --i) {
			if (q->led_state[i] == 1) {
				if (i+1 >= q->num_leds || 10 == q->led_state[i+1]) {
					q->led_state[i] = 10;
					break;
				} else {
					leds_0[i+1] = leds_0[i];
					leds_1[i+1] = leds_1[i];
					leds_0[i] = CRGB::Black;
					leds_1[i] = CRGB::Black;
					q->led_state[i] = 0;
					q->led_state[i+1] = 1;
				}
			}
		}
		if (10 == q->led_state[0]) {
			q->state = 4;
		} else if (q->preparing >= 0) {
			q->state = 0;
		}
		break;
	case 4: /* ledq done */
		if (q->tick++ > 100) {
			q->state = 5;
			q->tick = 1;
		}
		break;
	case 5: /* rotate pattern */
		if (0 == (q->tick % 3)) {
			tmp_led0 = leds_0[0];
			tmp_led1 = leds_1[0];
			for (i = 0; i < q->num_leds-1; ++i) {
				leds_0[i] = leds_0[i+1];
				leds_1[i] = leds_1[i+1];
			}
			leds_0[q->num_leds-1] = tmp_led0;
			leds_1[q->num_leds-1] = tmp_led1;
		}
		if (q->tick++ > 9*q->num_leds) {
			/* prepare shadow leds for partial fade */
			for (i = 0; i < q->num_leds; ++i) {
				sleds_0[i] = leds_0[i];
				sleds_1[i] = leds_1[i];
			}
			q->state = 6;
			q->tick = 0;
		}
		break;
	case 6: /* partial fade out from 0 */
		if (0 == (q->tick % 4)) {
			/* fade from 0 */
			for (q->pos = 0; q->pos < q->num_leds; ++q->pos) {
				if (2 == q->led_state[q->pos]) {
					leds_0[q->pos] /= 2;
					leds_1[q->pos] /= 2;
				} else if (10 == q->led_state[q->pos]) {
					q->led_state[q->pos] = 2;
					break;
				}
			}
		}
		if (q->tick++ > 4*q->num_leds) {
			for (i = 0; i < q->num_leds; ++i) {
				q->led_state[i] = 2;
			}
			q->state = 7;
			q->tick = 1;
		}
		break;
	case 7: /* partial fade in from max led */
		if (0 == (q->tick % 4)) {
			/* fade from 0 */
			for (q->pos = q->num_leds-1; q->pos >= 0; --q->pos) {
				if (q->led_state[q->pos] < 10) {
					leds_0[q->pos] = sleds_0[q->pos] / (10-q->led_state[q->pos]);
					leds_1[q->pos] = sleds_1[q->pos] / (10-q->led_state[q->pos]);
					if (2 == q->led_state[q->pos]++) break;
				}
			}
		}
		if (q->tick++ > 4*q->num_leds) {
			for (i = 0; i < q->num_leds; ++i) {
				q->led_state[i] = 10;
			}
			q->state = q->reps++ > 2 ? 8 : 6;
			q->tick = 1;
		}
		break;
	case 8: /* clear led strips */
		if (0 == q->tick % 4) {
			for (i = 0; i < q->num_leds; ++i) {
				leds_0[i] /= 2;
				leds_1[i] /= 2;
			}
		}
		if (++q->tick > 32) {
			for (i = 0; i < q->num_leds; ++i) {
				leds_0[i] = CRGB::Black;
				leds_1[i] = CRGB::Black;
				q->led_state[i] = 0;
			}
			q->tick = 1;
			q->state = 0;
		}
	default:
		break;
	};

//	ESP_LOGI(LOGTAG_LC, "q->state: %d, q->tick: %d, q->pos: %d, palette: %d, color: %d\n", q->state, q->tick, q->pos, lc_config.color_palette, lc_config.color);
}

