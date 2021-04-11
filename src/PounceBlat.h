#pragma once

#include "EventQueue.h"
#include "Relay.h"
#include "Sensor.h"

class PounceBlat {
	public:
		PounceBlat();
		void run();
	private:
		Relay relay_;
		Sensor sensor_;
		EventQueue eq_;

		enum State { ARMED, RUNNING } state_;

		void transitionTo(State s);
};

