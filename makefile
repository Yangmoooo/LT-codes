CC = g++
CFLAGS =

ENCODE_SRC = %_seed/encode.cpp %_seed/fountain.cpp
DECODE_SRC = %_seed/decode.cpp %_seed/fountain.cpp

%_encoder.exe: $(ENCODE_SRC)
	$(CC) $(CFLAGS) $^ -o $@

%_decoder.exe: $(DECODE_SRC)
	$(CC) $(CFLAGS) $^ -o $@

static: static_encoder.exe static_decoder.exe

dynamic: dynamic_encoder.exe dynamic_decoder.exe

clean:
	del *.exe encode.txt decode.*
