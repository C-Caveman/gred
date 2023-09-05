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
	# Put the binary in /usr/local/bin
	sudo cp gred /usr/local/bin/gred
	# Put gred_tutorial.txt in ~/.dotfiles/gred
	mkdir -p ${HOME}/.dotfiles
	mkdir -p ${HOME}/.dotfiles/gred
	cp documentation/gred_tutorial.txt ${HOME}/.dotfiles/gred
	# Put the manual file in /usr/share/man/man1
	sudo mkdir -p /usr/share/man/man1
	sudo cp documentation/gred.1 /usr/share/man/man1
