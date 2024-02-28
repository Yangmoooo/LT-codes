CC = clang
CFLAGS =
CXX = clang++
CXXFLAGS =

ifeq ($(OS),Windows_NT)
    SUFFIX = .exe
else
    SUFFIX = .out
endif

STATIC_TARGET = encoder-static$(SUFFIX) decoder-static$(SUFFIX)
DYNAMIC_TARGET = encoder-dynamic$(SUFFIX) decoder-dynamic$(SUFFIX)

static: $(STATIC_TARGET)
dynamic: $(DYNAMIC_TARGET)

encoder-%$(SUFFIX): encode.o %.o
	$(CXX) $^ -o $@

decoder-%$(SUFFIX): decode.o %.o
	$(CXX) $^ -o $@

encode.o: encode.c
	$(CC) $(CFLAGS) -c $< -o $@

decode.o: decode.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
ifeq ($(OS),Windows_NT)
	del /Q *$(SUFFIX) 2>nul
	del /Q *.o 2>nul
	del /Q .\data\encode.bin 2>nul
	del /Q .\data\decode.* 2>nul
else
	rm -f *$(SUFFIX) *.o ./data/encode.bin ./data/decode.*
endif

.PHONY: static dynamic clean