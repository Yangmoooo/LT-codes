# CC = g++
CC = clang++
CFLAGS =

ENCODE_SRC = encode.c fountain_%.cpp
DECODE_SRC = decode.c fountain_%.cpp

ifeq ($(OS),Windows_NT)
    SUFFIX = .exe
else
    SUFFIX = .out
endif

STATIC_TARGET = static_encoder$(SUFFIX) static_decoder$(SUFFIX)
DYNAMIC_TARGET = dynamic_encoder$(SUFFIX) dynamic_decoder$(SUFFIX)

%_encoder$(SUFFIX): $(ENCODE_SRC)
	$(CC) $(CFLAGS) $^ -o $@

%_decoder$(SUFFIX): $(DECODE_SRC)
	$(CC) $(CFLAGS) $^ -o $@

static: $(STATIC_TARGET)

dynamic: $(DYNAMIC_TARGET)

clean:
ifeq ($(OS),Windows_NT)
	del /Q *$(SUFFIX) 2>nul
	del /Q .\data\encode.bin 2>nul
	del /Q .\data\decode.* 2>nul
else
	rm -f *$(SUFFIX) ./data/encode.bin ./data/decode.*
endif
