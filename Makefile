default: build/teruez

build/teruez: build/uri.o build/buffer.o build/teruez.o build
	gcc build/uri.o build/buffer.o build/teruez.o -o build/teruez

build/uri.o: uri.c uri.h build
	gcc -c uri.c -o build/uri.o

build/buffer.o: buffer.c buffer.h build
	gcc -c buffer.c -o build/buffer.o

build/teruez.o: teruez.c uri.h buffer.h build
	gcc -c teruez.c -o build/teruez.o 

build:
	mkdir -p build

clean:
	rm -rf build

