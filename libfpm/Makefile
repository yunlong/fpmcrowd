MAJOR=1
MINOR=0
VERSION=$(MAJOR).$(MINOR)
#TARGETAR=libfpm.a
TARGETSO=libfpm.so

CC=gcc -O2 -g 

ARCH=
AR=ar
LD=ld
RANLIB=ranlib
LINK=ln -sf
CFLAGS= -DNDEBUG -D_UNIX -D_LINUX -D_REENTRANT -Wall -fPIC  -I. -I/usr/include/libxml2 -I /usr/include/  -I../libevt -DHAVE_CONFIG_H 

ARFLAGS=-rcs
LDFLAGS=$(ARCH) -shared -Wl,-soname,$(TARGETSO).$(MAJOR) -lxml2 -L../libevt -levt -o 

SRCS= $(wildcard *.c) $(wildcard *.cpp)
OBJS=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRCS)))

.SUFFIXES: .c .cpp

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY : all clean

all: $(TARGETAR) $(TARGETSO)

$(TARGETAR): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)
	$(RANLIB) $(TARGETAR)

$(TARGETSO): $(OBJS)
	$(CC) $(LDFLAGS) $(TARGETSO).$(VERSION) $(OBJS)
	$(LINK) $(TARGETSO).$(VERSION) $(TARGETSO).$(MAJOR)
	$(LINK) $(TARGETSO).$(MAJOR) $(TARGETSO)

clean:
	rm -f $(TARGETAR) $(TARGETSO) $(TARGETSO).$(VERSION) $(TARGETSO).$(MAJOR) $(OBJS) core
