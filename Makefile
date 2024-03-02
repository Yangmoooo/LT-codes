CC = clang
CFLAGS = -O0 -g -Wall
CXX = clang++
CXXFLAGS = -O0 -g -Wall

ifeq ($(OS),Windows_NT)
    SUFFIX = .exe
else
    SUFFIX = .out
endif

VPATH = src

TARGET_STATIC = encoder-static$(SUFFIX) decoder-static$(SUFFIX)
TARGET_DYNAMIC = encoder-dynamic$(SUFFIX) decoder-dynamic$(SUFFIX)

static: $(TARGET_STATIC)
dynamic: $(TARGET_DYNAMIC)

encoder-%$(SUFFIX): encode.o %.o
	$(CXX) $^ -o $@

decoder-%$(SUFFIX): decode.o %.o
	$(CXX) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
ifeq ($(OS),Windows_NT)
	del /Q *$(SUFFIX) 2>nul
	del /Q *.o 2>nul
	del /Q .\data\*.bin 2>nul
else
	rm -f *$(SUFFIX) *.o data/*.bin
endif

.PHONY: static dynamic clean
