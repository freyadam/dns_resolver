CC=clang++-9
CFLAGS=--std=c++17 -Wall -O2
LDFLAGS=

SRC=src
OBJ=obj

SOURCES=$(wildcard $(SRC)/*.cpp)
OBJECTS=$(SOURCES:$(SRC)/%.cpp=$(OBJ)/%.o)

.PHONY: all clean

all: bin/dns_resolver

clean:
	rm obj/*
	rm bin/*

bin/dns_resolver: $(OBJECTS)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(OBJECTS): $(OBJ)/%.o : $(SRC)/%.cpp
	@mkdir -p $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@
