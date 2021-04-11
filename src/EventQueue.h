#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>


class Event {
	public:
		enum { MOTION_DETECTED, TIMEOUT } type;
};

class EventQueue {
	public:
		EventQueue() {}
		~EventQueue() {}

		int send(Event const&);
		Event wait();

		void setTimeout(std::chrono::milliseconds delay) {
			deadline_ = std::chrono::steady_clock::now() +
				delay;
		}
		void clearTimeout() {
			deadline_ = std::nullopt;
		}

	private:
		std::deque<Event> queue_;
		std::mutex lock_;
		std::condition_variable cv_;
		std::optional<std::chrono::time_point<std::chrono::steady_clock>> deadline_;

};
