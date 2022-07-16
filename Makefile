DESTDIR    =
PREFIX     =/usr/local
AR         =ar
CC         =gcc
CFLAGS     =-Wall -g -DRELEASE
PROGRAMS   =imagestore$(EXE)
LIBRARIES  =libimagestore.a
HEADERS    =imagestore.h
CFLAGS_ALL =$(LDFLAGS) $(CFLAGS) $(CPPFLAGS)

##
all: $(PROGRAMS) $(LIBRARIES)
install: all
	install -d                  $(DESTDIR)$(PREFIX)/bin
	install -m755 $(PROGRAMS)   $(DESTDIR)$(PREFIX)/bin
	install -d                  $(DESTDIR)$(PREFIX)/include
	install -m644 $(HEADERS)    $(DESTDIR)$(PREFIX)/include
	install -d                  $(DESTDIR)$(PREFIX)/lib
	install -m644 $(LIBRARIES)  $(DESTDIR)$(PREFIX)/lib
clean:
	rm -f $(PROGRAMS) $(LIBRARIES)

##
imagestore$(EXE): main.c libimagestore.a $(HEADERS)
	$(CC) -o $@ main.c libimagestore.a $(CFLAGS_ALL)
libimagestore.a: imagestore.c $(HEADERS)
	$(CC) -c imagestore.c $(CFLAGS_ALL)
	$(AR) -crs $@ imagestore.o
	rm -f imagestore.o

## -- manpages --
ifneq ($(PREFIX),)
MAN_3=./imagestore.3 
install: install-man3
install-man3: $(MAN_3)
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man3
	cp $(MAN_3) $(DESTDIR)$(PREFIX)/share/man/man3
endif
## -- manpages --
## -- license --
ifneq ($(PREFIX),)
install: install-license
install-license: LICENSE
	mkdir -p $(DESTDIR)$(PREFIX)/share/doc/c-imagestore
	cp LICENSE $(DESTDIR)$(PREFIX)/share/doc/c-imagestore
endif
## -- license --
