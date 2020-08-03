#ifndef _MODE_BASE_H
#define _MODE_BASE_H

#include "alx_types.h"

class Mode_Base {
public:
	Mode_Base() {}

	virtual void run(ledq_t *q, lc_config_t *lc_config) {};
	virtual boolean init();
	virtual void helper() {};
	virtual boolean is_stable();
};

#endif
