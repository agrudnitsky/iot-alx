#include "alx.h"
#include "mode_power_updown.h"

bool Mode_Power_Down::operator()(int &b) {
	if (0 == (tick % stride)) {
		b = dim8_lin(b);
	}
	++tick;

	if (b <= 1) {
		tick = 0;
		return true;
	} else {
		return false;
	}
}
