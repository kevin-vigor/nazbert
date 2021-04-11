#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>


class Event {
	public:
		enum { MOTION_DETECTED } type;
};

class EventQueue {
	public:
		EventQueue() {}
		~EventQueue() {}

		int send(Event const&);
		Event wait();

	private:
		std::deque<Event> queue_;
		std::mutex lock_;
		std::condition_variable cv_;

};
