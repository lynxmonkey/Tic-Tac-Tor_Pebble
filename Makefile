CC ?= gcc
CORE = pebble/src/c/core
CFLAGS = -std=c99 -Wall -Wextra -Werror -O1 -g -I$(CORE) -Ipebble/src/c/render

test: build/test_core build/test_torus
	./build/test_core
	./build/test_torus

build/test_core: tests/test_core.c $(CORE)/*.c $(CORE)/*.h
	@mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined tests/test_core.c $(CORE)/ttt_board.c $(CORE)/ttt_game.c $(CORE)/ttt_ai.c $(CORE)/ttt_persist.c -o $@

build/test_torus: tests/test_torus.c pebble/src/c/render/torus_geom.c pebble/src/c/render/torus_geom.h
	@mkdir -p build
	$(CC) $(CFLAGS) -fsanitize=address,undefined tests/test_torus.c pebble/src/c/render/torus_geom.c -lm -o $@

clean:
	rm -rf build
.PHONY: test clean
