SHELL		= /bin/sh
A		= $(shell basename `pwd`)
CC		= gcc
# CFLAGS 
CFLAGS		= -Wall -O2 -g 
CFLAGS_GTK 	= $(shell pkg-config --silence-errors --cflags gtk+-2.0)
CFLAGS_GMODULE	= $(shell pkg-config --silence-errors --cflags gmodule-2.0)
CFLAGS_GLADE	= $(shell pkg-config --silence-errors --cflags libglade-2.0)
CFLAGS_NOTIFY	= $(shell pkg-config --silence-errors --cflags libnotify dbus-1)
# LIBS
LIBS_USB	= $(shell pkg-config --silence-errors --libs libusb)
LIBS_GTK 	= $(shell pkg-config --silence-errors --libs gtk+-2.0)
LIBS_GMODULE	= $(shell pkg-config --silence-errors --libs gmodule-2.0)
LIBS_GLADE	= $(shell pkg-config --silence-errors --libs libglade-2.0)
LIBS_NOTIFY	= $(shell pkg-config --silence-errors --libs libnotify)

DST		= gwsl2
PREFIX		= /usr/local
EMPTY		=
GLADE_PATH	= $(PREFIX)/share/gwsl2/


$(info Checking dependencies)
# Check,and print error if not found.
ifeq ($(CFLAGS_GTK),$(EMPTY))
$(error **ERROR** Gtk+-2.0 not found")
else
$(info **INFO** Gtk+-2.0 found)
endif
ifeq ($(CFLAGS_GMODULE),$(EMPTY))
$(error **ERROR** gmodule-2.0 not found")
else
$(info **INFO** gmodule-2.0 found)
endif
ifeq ($(LIBS_USB), $(EMPTY))
$(error **ERROR** Libusb Not found)
else
$(info **INFO** libusb found)
endif
ifeq ($(CFLAGS_GLADE),$(EMPTY))
$(error **ERROR** libglade-2.0 not found)
else
$(info **INFO** libglade-2.0 found )
endif
ifeq ($(CFLAGS_NOTIFY),$(EMPTY))
$(error **ERROR** libnotify not found)
else
$(info **INFO** libnotify found)
endif

$(info Dependencies found, continue)
CFLAGS_TOTAL =$(CFLAGS) $(CFLAGS_GTK) $(CFLAGS_USB) $(CFLAGS_GLADE) $(CFLAGS_NOTIFY) $(CFLAGS_GMODULE)
LIBS_TOTAL = $(LIBS) $(LIBS_USB) $(LIBS_GTK) $(LIBS_NOTIFY) $(LIBS_GLADE) $(LIBS_GMODULE)


all: $(DST)

wsl-main.c: wsl.gob
	$(info Creating wsl-main files)
	gob2 wsl.gob

wsl-transmitter.c: wlstransmitter.gob
	$(info Creating wls-transmitter files)
	gob2 wlstransmitter.gob

$(DST):	Makefile gwsl.c wsl-main.c wsl-transmitter.c 
	$(CC) $(CFLAGS_TOTAL)  -o $(DST) gwsl.c wsl-main.c  wsl-transmitter.c eggtrayicon.c $(LIBS_TOTAL) -DGLADE_PATH=\"$(GLADE_PATH)\"

clean:
	rm -f $(DST) *.o *~
	rm wsl-main*.* wsl-transmitter*.*

distclean:	clean
	rm -fr .deps

install:	$(DST)
	@(if test `id -u` -eq 0; then \
		strip $(DST); \
		install $(DST) $(PREFIX)/bin/$(DST); \
		install -D glade/gwsl.glade $(GLADE_PATH)/gwsl.glade;\
	else \
		echo "You must be root to install this ..."; \
	fi)

archive:	distclean
	(cd ..; tar czf $(A).tar.gz $(A))
