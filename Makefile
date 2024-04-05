CC = clang
CFLAGS = -O0 -g -Wall -Isrc
CXX = clang++
CXXFLAGS = -O0 -g -Wall -Isrc

VPATH = src

ifeq ($(OS),Windows_NT)
	EXT = .exe
else
	EXT =
endif

all: static dynamic clean
static: encoder-static$(EXT) decoder-static$(EXT)
dynamic: encoder-dynamic$(EXT) decoder-dynamic$(EXT)

encoder-%$(EXT): %.o encode.o utils.o
	$(CXX) $^ -o $@

decoder-%$(EXT): %.o decode.o utils.o
	$(CXX) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
ifeq ($(OS),Windows_NT)
	del /Q *.o 2>nul
	del /Q .\data\*.bin 2>nul
else
	rm -f *.o data/*.bin
endif

.PHONY: all static dynamic clean
