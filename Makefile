CC=g++
CFLAGS=-std=c++11

build: MyBot

MyBot: MyBot.o
	$(CC) $(CFLAGS) MyBot.o -o MyBot

MyBot.o: MyBot.cpp
	$(CC) $(CFLAGS) -c MyBot.cpp

run: build
	./MyBot

clean:
	rm -f MyBot MyBot.o
