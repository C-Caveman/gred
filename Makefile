# See LICENSE file for copyright and license details.
SOURCES = \
src/gred.c \
src/keybinds.c \
src/debug_stuff.c \
src/colorize.c \
src/menus.c \
src/file_types.c \
src/undo_redo.c \
src/editing.c src/input.c \
src/screen.c

gred: ${SOURCES} src/gred.h Makefile
	gcc ${SOURCES} -o gred

install: gred
	sudo cp gred /usr/bin/gred
