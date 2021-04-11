#include "EventQueue.h"

int EventQueue::send(Event const& e) {
	{
		std::lock_guard<std::mutex> lock(lock_);
		queue_.push_back(e);
	}
	cv_.notify_one();
	return 0;
}

Event EventQueue::wait() {
	Event e;
	bool timeout = false;
	static constexpr Event timeoutEvent { .type = Event::Type::TIMEOUT };
	std::unique_lock<std::mutex> lock(lock_);
	if (this->queue_.empty()) {
		if (deadline_) {
			timeout = !cv_.wait_until(lock, 
						 *deadline_,
						  [this] () { return !this->queue_.empty(); });
		} else {
			cv_.wait(lock, 
				[this] () { return !this->queue_.empty(); });
		} 
	}

	if (timeout) {
		e = timeoutEvent;
	} else {
		e = std::move(queue_.front());
		queue_.pop_front();
	}
	return e;
}

#ifdef EVENT_QUEUE_TEST
#include <cassert>
#include <cstdio>
#include <unistd.h>
#include <thread>
int main(void) {
	EventQueue q;

	std::thread generator([&q]() {
			Event e { .type = Event::Type::MOTION_DETECTED };

			for (int i = 0; i < 5; ++i) {
				q.send(e);
				::sleep(1);
				}
			});

	for (int i = 0; i < 5; ++i) {
		Event e = q.wait();
		assert(e.type == Event::Type::MOTION_DETECTED);
		puts("Got one!");
	}
	generator.join();
	return 0;
}
#endif
