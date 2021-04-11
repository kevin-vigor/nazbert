#include "PounceBlat.h"

#include <iostream>
#include <unistd.h>

PounceBlat::PounceBlat() {
}

void PounceBlat::run() {
	sensor_.monitor(eq_);

	while (1) {
		Event e = eq_.wait();

		switch (e.type) {
			case Event::MOTION_DETECTED:
				std::cout << "Motion detected!\n";
				relay_.set(true);
				::sleep(5);
				relay_.set(false);
				break;
		}

	}
}
