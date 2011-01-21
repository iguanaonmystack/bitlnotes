all: notes.so

notes.so:
	gcc -o notes.so -Werror -fPIC -shared notes.c `PKG_CONFIG_PATH=/home/flexo/.local/bitlbee/lib/pkgconfig pkg-config --cflags bitlbee`

clean:
	rm -f notes.so

install:
	cp -f notes.so /home/flexo/.local/bitlbee/lib/bitlbee/
