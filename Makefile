CC = clang
CFLAGS = -O0 -g -Wall
CXX = clang++
CXXFLAGS = -O0 -g -Wall

VPATH = src

ifeq ($(OS),Windows_NT)
	EXT = .exe
else
	EXT =
endif

all: static dynamic nohamming clean
static: encoder-static$(EXT) decoder-static$(EXT)
dynamic: encoder-dynamic$(EXT) decoder-dynamic$(EXT)

encoder-%$(EXT): %.o encoder.o utils.o
	$(CXX) $^ -o $@

decoder-%$(EXT): %.o decoder.o utils.o
	$(CXX) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
ifeq ($(OS),Windows_NT)
	del /Q *.o 2>nul
	del /Q .\data\*.enc 2>nul
	del /Q .\data\*.dec 2>nul
else
	rm -f *.o data/*.enc data/*.dec
endif

.PHONY: all static dynamic clean
