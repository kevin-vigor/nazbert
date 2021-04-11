#include "PounceBlat.h"

#include <iostream>
#include <unistd.h>

#include <spdlog/spdlog.h>

PounceBlat::PounceBlat() : state_(ARMED) {
}

void PounceBlat::transitionTo(State s) {
	if (s != state_) {
		eq_.clearTimeout();
		state_ = s;
		switch (s) {
			case RUNNING:
				relay_.set(true);
				eq_.setTimeout(std::chrono::seconds(5)); // FIXME: configurable runtime?
			break;
			case ARMED:
				relay_.set(false);
			default:
				// nada
			break;
		}
	}
}

void PounceBlat::run() {
	sensor_.monitor(eq_);

	while (1) {
		Event e = eq_.wait();

		spdlog::debug("Received event {} in state {}", e, state_);

		switch (state_) {
			case ARMED:
				switch (e.type) {
					case Event::MOTION_DETECTED:
						std::cout << "Motion detected!\n";
						relay_.set(true);
						transitionTo(RUNNING);
					break;

					case Event::TIMEOUT:
						std::cout << "Unexpected event.\n";
					break;
				}
			break;

			case RUNNING:
				switch (e.type) {
					case Event::MOTION_DETECTED:
					case Event::TIMEOUT:
						transitionTo(ARMED);
					break;

				}
			break;
		}
	}
}
