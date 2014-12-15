default: build/teruez

build/teruez: build/buffer.o build/check.o build/teruez.o build/uri.o build
	gcc -g --std=c11 build/buffer.o build/check.o build/teruez.o build/uri.o -o build/teruez

build/buffer.o: buffer.c buffer.h build
	gcc -g --std=c11 -c buffer.c -o build/buffer.o

build/check.o: check.c check.h build
	gcc -g --std=c11 -c check.c -o build/check.o

build/teruez.o: teruez.c buffer.h check.h uri.h build
	gcc -g --std=c11 -c teruez.c -o build/teruez.o 

build/uri.o: uri.c uri.h build
	gcc -g --std=c11 -c uri.c -o build/uri.o

build:
	mkdir -p build

clean:
	rm -rf build

