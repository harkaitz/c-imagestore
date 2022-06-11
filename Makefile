## Configuration
DESTDIR    =
PREFIX     =/usr/local
AR         =ar
CC         =gcc
CFLAGS     =-Wall -g -DRELEASE
CPPFLAGS   =
LIBS       =
## Sources and targets
PROGRAMS   =imagestore
LIBRARIES  =libimagestore.a
HEADERS    =imagestore.h
MANPAGES_3 =imagestore.3

SOURCES_L  =imagestore.c
SOURCES_P  =main.c

## AUXILIARY
CFLAGS_ALL =$(LDFLAGS) $(CFLAGS) $(CPPFLAGS)

## STANDARD TARGETS
all: $(PROGRAMS) $(LIBRARIES)
help:
	@echo "all     : Build everything."
	@echo "clean   : Clean files."
	@echo "install : Install all produced files."
install: all
	install -d                  $(DESTDIR)$(PREFIX)/bin
	install -m755 $(PROGRAMS)   $(DESTDIR)$(PREFIX)/bin
	install -d                  $(DESTDIR)$(PREFIX)/include
	install -m644 $(HEADERS)    $(DESTDIR)$(PREFIX)/include
	install -d                  $(DESTDIR)$(PREFIX)/lib
	install -m644 $(LIBRARIES)  $(DESTDIR)$(PREFIX)/lib
	install -d                  $(DESTDIR)$(PREFIX)/share/man/man3
	install -m644 $(MANPAGES_3) $(DESTDIR)$(PREFIX)/share/man/man3
clean:
	rm -f $(PROGRAMS) $(LIBRARIES)

## TARGETS
imagestore: main.c libimagestore.a $(HEADERS)
	$(CC) -o $@ main.c libimagestore.a $(CFLAGS_ALL)
libimagestore.a: imagestore.c $(HEADERS)
	$(CC) -c imagestore.c $(CFLAGS_ALL)
	$(AR) -crs $@ imagestore.o
	rm -f imagestore.o
