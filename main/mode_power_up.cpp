#include "alx.h"
#include "mode_power_updown.h"

bool Mode_Power_Up::operator()(int &b, int max) {
	if (tick < power_up_duration) {
		if (0 == (tick % stride)) {
			if (b == 0) b = 1;
			b = 255-dim8_lin(255-b);
			b = b > max ? max : b;
		}
		++tick;
		return false;
	} else {
		b = max;
		tick = 0;
		return true;
	}
}


bool Mode_Power_Up::is_stable() {
	return false;
}
