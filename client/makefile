CFLAGS := -c -Wall -Weffc++ -g -std=c++11 -Iinclude
LDFLAGS := -lboost_system -lpthread

all: StompEMIClient

StompEMIClient: bin bin/ConnectionHandler.o bin/StompClient.o bin/event.o bin/keyboardInput.o bin/StompProtocol.o bin/main.o
	g++ -o bin/StompEMIClient bin/ConnectionHandler.o bin/StompClient.o bin/event.o bin/keyboardInput.o bin/StompProtocol.o bin/main.o $(LDFLAGS)

bin:
	mkdir -p bin

bin/ConnectionHandler.o: src/ConnectionHandler.cpp include/ConnectionHandler.h
	g++ $(CFLAGS) -o bin/ConnectionHandler.o src/ConnectionHandler.cpp

bin/event.o: src/event.cpp include/event.h
	g++ $(CFLAGS) -o bin/event.o src/event.cpp

bin/StompClient.o: src/StompClient.cpp include/StompClient.h
	g++ $(CFLAGS) -o bin/StompClient.o src/StompClient.cpp

bin/StompProtocol.o: src/StompProtocol.cpp include/StompProtocol.h
	g++ $(CFLAGS) -o bin/StompProtocol.o src/StompProtocol.cpp

bin/keyboardInput.o: src/keyboardInput.cpp include/keyboardInput.h
	g++ $(CFLAGS) -o bin/keyboardInput.o src/keyboardInput.cpp

bin/main.o: src/main.cpp
	g++ $(CFLAGS) -o bin/main.o src/main.cpp

.PHONY: clean run
clean:
	rm -f bin/*

run:
	./bin/StompEMIClient
