#ifndef _MODE_TDC_H
#define _MODE_TDC_H

#include "alx_types.h"
#include "mode_constant.h"

typedef struct {
	int secs_past_mn;
	int palette;
	int color;
	int brightness;
} tdc_entry_t;


class Mode_TDC: public Mode_Constant {
public:
	Mode_TDC() {}
	boolean init();
	void helper();
	void run(ledq_t *q, lc_config_t *lc_config);
private:
	void tdc_color_lookup();
	int num_time_colors;
	CRGB transient_color;
	tdc_entry_t *time_colors;
};


#endif
