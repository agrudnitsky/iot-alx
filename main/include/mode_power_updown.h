#ifndef _MODE_POWER_UPDOWN_H
#define _MODE_POWER_UPDOWN_H

#include "mode_base.h"

class Mode_Power_Up: public Mode_Base {
public:
	Mode_Power_Up(int _pud, int _st): tick(0), power_up_duration(_pud), stride(_st) {}

	bool operator()(int &b, int max);
	bool is_stable();

private:
	int tick;
	int power_up_duration;
	int stride;
};


class Mode_Power_Down: public Mode_Base {
public:
	Mode_Power_Down(int _st): tick(0), stride(_st) {}

	bool operator()(int &b);
	bool is_stable();

private:
	int tick;
	int stride;
};


#endif
