#ifndef _MODE_CONSTANT_H
#define _MODE_CONSTANT_H

#include "alx_types.h"
#include "mode_base.h"

class Mode_Constant: public Mode_Base {
public:
	Mode_Constant() {}

	void run(ledq_t *q, lc_config_t *lc_config);
protected:
	void set_all_leds(CRGB color);
};


#endif
