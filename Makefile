# See LICENSE file for copyright and license details.
SOURCES = \
src/gred.c \
src/keybinds.c \
src/commands.c \
src/debug_stuff.c \
src/colorize.c \
src/menus.c \
src/file_types.c \
src/editing.c \
src/input.c \
src/screen.c

gred: ${SOURCES} src/gred.h Makefile
	gcc ${SOURCES} -o gred -lpthread

install: gred
	sudo cp gred /usr/local/bin/gred     # Put the binary in /usr/local/bin            #
	
	mkdir -p ${HOME}/.dotfiles           # Put tutorial.txt in ~/.dotfiles/gred        #
	mkdir -p ${HOME}/.dotfiles/gred
	cp "guide/tutorial.txt" "${HOME}/.dotfiles/gred/"
	
	sudo mkdir -p /usr/share/man/man1    # Put the manual file in /usr/share/man/man1  #
	sudo cp guide/gred.1 /usr/share/man/man1
