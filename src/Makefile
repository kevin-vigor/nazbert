CXX = clang++
CXXFLAGS ?= --std=c++17 -Wall -Werror -O2

OBJECTS = Controller.o EventQueue.o Relay.o Sensor.o PounceBlat.o Scanner.o \
  main.o

LIBS = -lgpiodcxx -lpthread -lbluetooth

DEP = $(OBJECTS:%.o=%.d)

all: pounceblat

-include $(DEP)

clean:
	rm -f $(OBJECTS) $(DEP)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

pounceblat: $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LIBS)

scanner-test: EventQueue.o Scanner.cpp
	$(CXX) $(CXXFLAGS) -o $@ -DSCANNER_TEST Scanner.cpp EventQueue.o $(LIBS)

control-test: EventQueue.o Controller.cpp
	$(CXX) $(CXXFLAGS) -o $@ -DCONTROL_TEST Controller.cpp EventQueue.o  $(LIBS)
