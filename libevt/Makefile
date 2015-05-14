MAJOR=1
MINOR=0
VERSION=$(MAJOR).$(MINOR)
#TARGETAR=libevt.a
TARGETSO=libevt.so


#CC=gcc 
CC=gcc -O2 -g 
#CXX=g++ -O2 -g 
CXX=g++ -O2 -g -MT -MD -MP -MF 

ARCH=
AR=ar
LD=ld
RANLIB=ranlib
LINK=ln -sf
CFLAGS= -DNDEBUG -D_UNIX -D_LINUX -D_REENTRANT -Wall -fPIC  -I. -I /usr/include/   -I./compat -DHAVE_CONFIG_H
CXXFLAGS= -DNDEBUG -D_UNIX -D_LINUX -D_REENTRANT -Wall -fPIC  -I. -I /usr/include/ -I./compat -DHAVE_CONFIG_H

ARFLAGS=-rcs
#LDFLAGS=$(ARCH) -shared -Wl,-soname,$(TARGETSO).$(MAJOR) -lpthread  -o 
LDFLAGS=$(ARCH) -shared -Wl,-soname,$(TARGETSO).$(MAJOR) -o 

SRCS= $(wildcard *.c) $(wildcard *.cpp)
OBJS=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRCS)))

.SUFFIXES: .c .cpp

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

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
