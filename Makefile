default: build/teruez

CFLAGS=-g --std=c99 -Wall -Werror

build/teruez: build/buffer.o build/check.o build/teruez.o build/uri.o
	gcc -g --std=c99 build/buffer.o build/check.o build/teruez.o build/uri.o -o build/teruez

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

install:
	install build/teruez /usr/local/bin/teruez
	install teruez.service /usr/lib/systemd/system/teruez.service
	systemctl daemon-reload

uninstall:
	rm -rf /usr/local/bin/teruez
	rm -rf /usr/lib/systemd/system/teruez.service

