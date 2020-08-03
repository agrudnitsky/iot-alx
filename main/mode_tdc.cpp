#include "alx.h"
#include "mode_tdc.h"

void Mode_TDC::run(ledq_t *q, lc_config_t *lc_config) {
	set_all_leds(transient_color);
}


boolean Mode_TDC::init() {
	num_time_colors = 6;

	/* format: secs past midnight, palette, color_id, brightness */
	time_colors = malloc(num_time_colors * sizeof(tdc_entry_t));
	time_colors[0] = {( 0*60 + 30)*60, 1, 0,  90};
	time_colors[1] = {( 5*60 + 30)*60, 1, 1,  90};
	time_colors[2] = {( 8*60 + 30)*60, 1, 2, 200};
	time_colors[3] = {(12*60 + 30)*60, 1, 3, 240};
	time_colors[4] = {(16*60 + 30)*60, 1, 4, 240};
	time_colors[5] = {(21*60 + 30)*60, 1, 5, 140};

	return true;
}

void Mode_TDC::helper() {
	tdc_color_lookup();
}


void Mode_TDC::tdc_color_lookup() {
	int i;
	struct timeval now_te;
	time_t now_ts;
	struct tm *now;
	int secs_past_mn;
	int last_tdc_id = -1;
	int next_tdc_id = -1;
	int tdc_distance = 0;
	int secs_past_last_tdc;
	int time_progress;

	gettimeofday(&now_te, NULL);
	now_ts = now_te.tv_sec;
	now = localtime(&now_ts);
	secs_past_mn = now->tm_hour*3600 + now->tm_min*60 + now->tm_sec;
	ESP_LOGI(LOGTAG_MISC, "now: %d:%02d:%02d", now->tm_hour, now->tm_min, now->tm_sec);

	for (i = 0; i < num_time_colors; ++i) {
		if (secs_past_mn > time_colors[i].secs_past_mn) {
			last_tdc_id = i;
		}
	}
	if (-1 == last_tdc_id) last_tdc_id = num_time_colors-1;

	if (last_tdc_id == num_time_colors-1) {
		next_tdc_id = 0;
		tdc_distance = 24*60*60;
	} else {
		next_tdc_id = last_tdc_id + 1;
	}
	tdc_distance += time_colors[next_tdc_id].secs_past_mn - time_colors[last_tdc_id].secs_past_mn;

	secs_past_last_tdc = secs_past_mn - time_colors[last_tdc_id].secs_past_mn;
	if (secs_past_mn < time_colors[last_tdc_id].secs_past_mn) secs_past_last_tdc += 24*60*60;
	time_progress = (100*secs_past_last_tdc) / tdc_distance;

	/* interpolate brightness */
	lc_config.set_bright = time_colors[last_tdc_id].brightness + time_progress*(time_colors[next_tdc_id].brightness - time_colors[last_tdc_id].brightness)/100;

	lc_config.color_palette = time_colors[last_tdc_id].palette;
	lc_config.color = time_colors[last_tdc_id].color;

	/* interpolate color */
	/* XXX: not sure if we should do this in HSV
	 * this would probably require all LED operations to be done in HSV, though
	 */
	transient_color.red = color_palette[time_colors[last_tdc_id].palette][time_colors[last_tdc_id].color].red + time_progress*(color_palette[time_colors[next_tdc_id].palette][time_colors[next_tdc_id].color].red - color_palette[time_colors[last_tdc_id].palette][time_colors[last_tdc_id].color].red)/100;
	transient_color.green = color_palette[time_colors[last_tdc_id].palette][time_colors[last_tdc_id].color].green + time_progress*(color_palette[time_colors[next_tdc_id].palette][time_colors[next_tdc_id].color].green - color_palette[time_colors[last_tdc_id].palette][time_colors[last_tdc_id].color].green)/100;
	transient_color.blue = color_palette[time_colors[last_tdc_id].palette][time_colors[last_tdc_id].color].blue + time_progress*(color_palette[time_colors[next_tdc_id].palette][time_colors[next_tdc_id].color].blue - color_palette[time_colors[last_tdc_id].palette][time_colors[last_tdc_id].color].blue)/100;
}
