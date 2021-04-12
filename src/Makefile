CXX = clang++
CXXFLAGS ?= --std=c++17 -Wall -Werror -O2

OBJECTS = EventQueue.o Relay.o Sensor.o PounceBlat.o main.o

DEP = $(OBJECTS:%.o=%.d)

all: pounceblat

-include $(DEP)

clean:
	rm -f $(OBJECTS) $(DEP)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

pounceblat: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o pounceblat $(OBJECTS) -lgpiodcxx -lpthread


