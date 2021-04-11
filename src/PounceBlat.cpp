#include "PounceBlat.h"

#include <iostream>
#include <unistd.h>

#include <spdlog/spdlog.h>

PounceBlat::PounceBlat() : state_(State::ARMED) {
}

void PounceBlat::transitionTo(State s) {
	if (s != state_) {
		spdlog::debug("Transition state from {} -> {}", state_, s);
		eq_.clearTimeout();
		state_ = s;
		switch (s) {
			case State::RUNNING:
				relay_.set(true);
				eq_.setTimeout(std::chrono::seconds(5)); // FIXME: configurable runtime?
			break;
			case State::ARMED:
				relay_.set(false); // Should be a no-op... but can't hurt, eh?
			break;
			case State::GRACE:
				relay_.set(false);
				eq_.setTimeout(std::chrono::seconds(10)); 
			break;
		}
	} else {
		spdlog::warn("Attempted to transition to state {} when already in that state.");
	}
}

void PounceBlat::run() {
	sensor_.monitor(eq_);

	while (1) {
		Event e = eq_.wait();

		spdlog::debug("Received event {} in state {}", e, state_);

		switch (state_) {
			case State::ARMED:
				switch (e.type) {
					case Event::Type::MOTION_DETECTED:
						std::cout << "Motion detected!\n";
						relay_.set(true);
						transitionTo(State::RUNNING);
					break;

					case Event::Type::TIMEOUT:
						std::cout << "Unexpected event.\n";
					break;
				}
			break;

			case State::RUNNING:
				switch (e.type) {
					case Event::Type::MOTION_DETECTED:
						spdlog::debug("Motion ignored, already in RUNNING state.");
					break;
					case Event::Type::TIMEOUT:
						transitionTo(State::GRACE);
					break;

				}
			break;

			case State::GRACE:
				switch (e.type) {
					case Event::Type::MOTION_DETECTED:
						spdlog::debug("Event {} ignored in GRACE state.", e);
					break;
				 	case Event::Type::TIMEOUT:
						transitionTo(State::ARMED);
					break;
				}
			break;
		}
	}
}
