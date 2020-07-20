#ifndef _MODE_XMAS_H
#define _MODE_XMAS_H

#include "alx_types.h"

class Mode_XMAS {
public:
	Mode_XMAS() {}

	void flying_lights(ledq_t *q);
private:
	void prepare_next_light(ledq_t *q);
	uint32_t rand_bounded(unsigned int bound);
	int qsort_step(CHSV *l);

	CHSV pleds[NUM_LEDS];
	CHSV sleds[NUM_LEDS];
};


#endif
