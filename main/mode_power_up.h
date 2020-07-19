#ifndef _MODE_POWER_UP_H
#define _MODE_POWER_UP_H


class Mode_Power_Up {
public:
	Mode_Power_Up(int _pud, int _st): tick(0), power_up_duration(_pud), stride(_st) {}

	bool operator()(int &b, int max);

private:
	int tick;
	int power_up_duration;
	int stride;
};

#endif
