CXX ?= clang++
CXXFLAGS ?= --std=c++17 -Wall -Werror

OBJECTS = EventQueue.o Relay.o Sensor.o PounceBlat.o

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

pounceblat: $(OBJECTS)
	$(CXX) -o pounceblat main.cpp $(OBJECTS) -lgpiodcxx -lpthread


all: pounceblat
