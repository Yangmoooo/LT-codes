CC = clang
CFLAGS = -O0 -g -Wall -Isrc
CXX = clang++
CXXFLAGS = -O0 -g -Wall -Isrc

VPATH = src

static: encoder-static decoder-static
dynamic: encoder-dynamic decoder-dynamic

encoder-%: %.o encode.o utils.o
	$(CXX) $^ -o $@

decoder-%: %.o decode.o utils.o
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

.PHONY: static dynamic clean
