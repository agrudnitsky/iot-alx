#include "alx_types.h"

CHSV pleds[NUM_LEDS];
CHSV sleds[NUM_LEDS];

void prepare_next_light(ledq_t *q) {
	CHSV new_col = CHSV(rand_bounded(256), 255, 255);
	pleds[0] = new_col;

	q->led_state[0] = 1;
}


int qsort_step(CHSV *l) {
	return 1;
}


void flying_lights(ledq_t *q) {
	int i;
	CHSV tmp_led;

	switch (q->state) {
	case 0: /* prepare and flare up */
		q->reps = 0;
		if (0 == q->preparing) {
			prepare_next_light(q);
			q->preparing = 1;
		}
		if (q->preparing > 0) {
			/* flare up */
			if (0 == (q->preparing % 4)) {
				pleds[0].val = 255/(q->preparing/4);
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
					pleds[i+1] = pleds[i];
					pleds[i].val = 0;
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
			tmp_led = pleds[0];
			for (i = 0; i < q->num_leds-1; ++i) {
				pleds[i] = pleds[i+1];
			}
			pleds[q->num_leds-1] = tmp_led;
		}
		if (q->tick++ > 9*q->num_leds) {
			/* prepare shadow leds for partial fade */
			for (i = 0; i < q->num_leds; ++i) {
				sleds[i] = pleds[i];
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
					pleds[q->pos].val /= 2;
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
					pleds[q->pos].val = 255 / (10-q->led_state[q->pos]);
					if (2 == q->led_state[q->pos]++) break;
				}
			}
		}
		if (q->tick++ > 4*q->num_leds) {
			for (i = 0; i < q->num_leds; ++i) {
				q->led_state[i] = 10;
			}
			q->state = q->reps++ > 1 ? 8 : 6;
			q->tick = 1;
		}
		break;
	case 8: /* sort leds */
		if (1 == qsort_step(pleds)) {
			q->tick = 1;
			q->state = 9;
		}
		break;
	case 9: /* clear led strips */
		if (0 == q->tick % 4) {
			for (i = 0; i < q->num_leds; ++i) {
				pleds[i].val /= 2;
			}
		}
		if (++q->tick > 32) {
			for (i = 0; i < q->num_leds; ++i) {
				pleds[i].val = 0;
				q->led_state[i] = 0;
			}
			q->tick = 1;
			q->state = 0;
		}
	default:
		break;
	};

	for (i = 0; i < q->num_leds; ++i) {
		leds_0[i] = pleds[i];
		leds_1[i] = pleds[i];
	}
//	ESP_LOGI(LOGTAG_LC, "q->state: %d, q->tick: %d, q->pos: %d, palette: %d, color: %d\n", q->state, q->tick, q->pos, lc_config.color_palette, lc_config.color);
}

