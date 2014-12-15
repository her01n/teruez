default: build/teruez

CFLAGS=-g --std=c11

build/teruez: build/buffer.o build/check.o build/teruez.o build/uri.o
	gcc -g --std=c11 build/buffer.o build/check.o build/teruez.o build/uri.o -o build/teruez

build/buffer.o: buffer.c buffer.h
	mkdir -p build
	gcc $(CFLAGS) -c buffer.c -o build/buffer.o

build/check.o: check.c check.h
	mkdir -p build
	gcc $(CFLAGS) -c check.c -o build/check.o

build/teruez.o: teruez.c buffer.h check.h uri.h
	mkdir -p build
	gcc $(CFLAGS) -c teruez.c -o build/teruez.o 

build/uri.o: uri.c uri.h
	mkdir -p build
	gcc $(CFLAGS) -c uri.c -o build/uri.o

clean:
	rm -rf build

