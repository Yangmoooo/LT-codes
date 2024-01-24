CC = g++
CFLAGS = -O2

ENCODE_SRC = encode.c fountain_%.cpp
DECODE_SRC = decode.c fountain_%.cpp
STATIC_EXE = static_encoder.exe static_decoder.exe
DYNAMIC_EXE = dynamic_encoder.exe dynamic_decoder.exe

%_encoder.exe: $(ENCODE_SRC)
	$(CC) $(CFLAGS) $^ -o $@

%_decoder.exe: $(DECODE_SRC)
	$(CC) $(CFLAGS) $^ -o $@

static: $(STATIC_EXE)

dynamic: $(DYNAMIC_EXE)

clean:
	del *.exe 2>nul & del /Q .\output\* 2>nul
