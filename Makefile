CC=g++
CFLAGS=--std=c++17 -Wall -O2
LDFLAGS=

SOURCES=$(wildcard $(SRC)/*.cpp)
OBJECTS=$(SOURCES:$(SRC)/%.cpp=$(OBJ)/%.o)

.PHONY: all clean

all: bin/dns_resolver

clean:
	rm obj/*
	rm bin/*

bin/dns_resolver: obj/resolver.o obj/main.o
	@mkdir -p bin
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(OBJECTS): $(OBJ)/%.o : $(SRC)/%.cpp
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

obj/resolver.o: src/resolver.cpp
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

obj/main.o: src/main.cpp
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@
