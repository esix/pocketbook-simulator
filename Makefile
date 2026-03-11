CC = emcc
CFLAGS = -Iinclude -s WASM=1 -s INVOKE_RUN=0
LIBS =
EXPORTED_FUNCTIONS = -s EXPORTED_FUNCTIONS='["_main", "_malloc"]'
EXPORTED_RUNTIME = -s EXPORTED_RUNTIME_METHODS='["UTF8ToString","wasmTable","lengthBytesUTF8","stringToUTF8"]'
ADDITIONAL_LIBS = -s USE_FREETYPE=1 -s USE_ZLIB=1
MODULARIZE = -s MODULARIZE=1 -s 'EXPORT_NAME="createPocketBookModule"' -s EXPORT_ES6=1 -O3 -s AGGRESSIVE_VARIABLE_ELIMINATION=1

all: demo01 demo04 demo06 demo07 calc touch helloworld game2048 tictactoe pbtetris

lib/inkview.o: src/inkview.c
	$(CC) $(CFLAGS) -c $< -o $@ $(ADDITIONAL_LIBS) $(EXPORTED_FUNCTIONS) $(EXPORTED_RUNTIME)

demo01: projects/demo01/demo01.c lib/inkview.o
	$(CC) $(CFLAGS) projects/demo01/demo01.c lib/inkview.o -o projects/$@/index.mjs $(EXPORTED_FUNCTIONS) $(EXPORTED_RUNTIME) $(ADDITIONAL_LIBS) $(MODULARIZE)

calc: projects/calc/calcexe.c lib/inkview.o
	$(CC) $(CFLAGS) projects/calc/calcexe.c lib/inkview.o -o projects/$@/index.mjs $(EXPORTED_FUNCTIONS) $(EXPORTED_RUNTIME) $(ADDITIONAL_LIBS) $(MODULARIZE)

touch: projects/touch/touch.c lib/inkview.o
	$(CC) $(CFLAGS) projects/touch/touch.c lib/inkview.o -o projects/$@/index.mjs $(EXPORTED_FUNCTIONS) $(EXPORTED_RUNTIME) $(ADDITIONAL_LIBS) $(MODULARIZE)

HELLOWORLD_SRC = projects/helloworld/src/main.cpp \
                 projects/helloworld/src/handler/eventHandler.cpp \
                 projects/helloworld/src/handler/menuHandler.cpp \
                 projects/helloworld/src/ui/basicView.cpp

helloworld: $(HELLOWORLD_SRC) lib/inkview.o
	$(CC) $(CFLAGS) \
		-Iprojects/helloworld/src/handler \
		-Iprojects/helloworld/src/ui \
		$(HELLOWORLD_SRC) lib/inkview.o \
		-o projects/$@/index.mjs \
		$(EXPORTED_FUNCTIONS) $(EXPORTED_RUNTIME) $(ADDITIONAL_LIBS) $(MODULARIZE)

game2048: projects/2048/game2048.c lib/inkview.o
	$(CC) $(CFLAGS) projects/2048/game2048.c lib/inkview.o -o projects/2048/index.mjs $(EXPORTED_FUNCTIONS) $(EXPORTED_RUNTIME) $(ADDITIONAL_LIBS) $(MODULARIZE)

TICTACTOE_SRC = projects/tictactoe/src/main.cpp \
                projects/tictactoe/src/handler/eventHandler.cpp \
                projects/tictactoe/src/handler/menuHandler.cpp \
                projects/tictactoe/src/ui/game.cpp \
                projects/tictactoe/src/ui/field.cpp

tictactoe: $(TICTACTOE_SRC) lib/inkview.o
	$(CC) $(CFLAGS) \
		-Iprojects/tictactoe/src/handler \
		-Iprojects/tictactoe/src/ui \
		$(TICTACTOE_SRC) lib/inkview.o \
		-o projects/$@/index.mjs \
		$(EXPORTED_FUNCTIONS) $(EXPORTED_RUNTIME) $(ADDITIONAL_LIBS) $(MODULARIZE)

PBTETRIS_SRC = projects/pbtetris/src/main.cpp \
               projects/pbtetris/src/game.cpp \
               projects/pbtetris/src/tetris.cpp \
               projects/pbtetris/src/buttons.cpp \
               projects/pbtetris/src/highscore.cpp \
               projects/pbtetris/src/debug.cpp \
               projects/pbtetris/cimages/images.c

pbtetris: $(PBTETRIS_SRC) lib/inkview.o
	$(CC) $(CFLAGS) \
		-Iprojects/pbtetris/src \
		$(PBTETRIS_SRC) lib/inkview.o \
		-o projects/$@/index.mjs \
		$(EXPORTED_FUNCTIONS) $(EXPORTED_RUNTIME) $(ADDITIONAL_LIBS) $(MODULARIZE)

demo04: projects/demo04/demo04.cpp lib/inkview.o
	$(CC) $(CFLAGS) projects/demo04/demo04.cpp lib/inkview.o -o projects/$@/index.mjs $(EXPORTED_FUNCTIONS) $(EXPORTED_RUNTIME) $(ADDITIONAL_LIBS) $(MODULARIZE)

demo06: projects/demo06/demo06.cpp lib/inkview.o
	$(CC) $(CFLAGS) projects/demo06/demo06.cpp lib/inkview.o -o projects/$@/index.mjs $(EXPORTED_FUNCTIONS) $(EXPORTED_RUNTIME) $(ADDITIONAL_LIBS) $(MODULARIZE)

demo07: projects/demo07/demo07.cpp lib/inkview.o
	$(CC) $(CFLAGS) projects/demo07/demo07.cpp lib/inkview.o -o projects/$@/index.mjs $(EXPORTED_FUNCTIONS) $(EXPORTED_RUNTIME) $(ADDITIONAL_LIBS) -s USE_SQLITE3=1 $(MODULARIZE)

clean:
	rm -f lib/*.o lib/*.a projects/demo01/*.mjs projects/demo01/*.wasm \
		projects/demo04/*.mjs projects/demo04/*.wasm \
		projects/demo06/*.mjs projects/demo06/*.wasm \
		projects/demo07/*.mjs projects/demo07/*.wasm \
		projects/calc/*.mjs projects/calc/*.wasm \
		projects/touch/*.mjs projects/touch/*.wasm \
		projects/helloworld/*.mjs projects/helloworld/*.wasm \
		projects/2048/*.mjs projects/2048/*.wasm \
		projects/tictactoe/*.mjs projects/tictactoe/*.wasm \
		projects/pbtetris/*.mjs projects/pbtetris/*.wasm

.PHONY: all clean

