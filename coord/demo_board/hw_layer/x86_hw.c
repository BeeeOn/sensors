#include "include.h"

void delay_ms(uint16_t t) {
	std::this_thread::sleep_for(std::chrono::milliseconds(t));
}
