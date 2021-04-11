CXX ?= clang++
CXXFLAGS ?= --std=c++17 -Wall -Werror -O2

OBJECTS = EventQueue.o Relay.o Sensor.o PounceBlat.o

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

pounceblat: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o pounceblat main.cpp $(OBJECTS) -lgpiodcxx -lpthread


all: pounceblat
