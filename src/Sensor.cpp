#include "Sensor.h"

Sensor::Sensor() : chip_("gpiochip0") {
	terminating_ = false;

	line_ = chip_.get_line(4);

	::gpiod::line_request req;
	req.consumer = "nazbert";
	req.request_type = ::gpiod::line_request::EVENT_RISING_EDGE; 
	req.flags = 0;

	line_.request(req);
}

Sensor::~Sensor() {
	if (monitorThread_.joinable()) {
		terminating_ = true;
		monitorThread_.join();
	}
}

void Sensor::monitor(EventQueue& eq) {
	if (monitorThread_.joinable()) {
		throw std::runtime_error("monitor can only be called once.");
	}
	monitorThread_ = std::thread([&eq, this]() {
		const Event mdEvent { .type = Event::MOTION_DETECTED };
		while (!this->terminating_) {

			if (this->line_.event_wait(::std::chrono::seconds(1))) {
				auto e = this->line_.event_read();
				switch (e.event_type) {
					case ::gpiod::line_event::RISING_EDGE:
						eq.send(mdEvent);
						break;
					default:
						throw std::runtime_error("Unexpected event!");
						break;
					
				}
			}

		}
	});
}

#ifdef SENSOR_TEST 
#include <iostream>
int main(void) {
	EventQueue eq;
	Sensor s;

	s.monitor(eq);

	while (1) {
		Event e = eq.wait();
		switch (e.type) {
			case Event::MOTION_DETECTED:
				std::cout << "Motion detected!\n";
				break;
			default:
				std::cout << "WTF??\n";
				break;
		}

	}
	return 1;
}
#endif
