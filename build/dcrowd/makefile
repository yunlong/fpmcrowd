MAJOR=1
MINOR=0
VERSION=$(MAJOR).$(MINOR)
TARGETSO=libcrowd.so

CXX=g++ -O2 -g 

ARCH=
AR=ar
LD=ld
RANLIB=ranlib
LINK=ln -sf
CXXFLAGS= -DNDEBUG -D_UNIX -D_LINUX -D_REENTRANT -Wall -fPIC  -I. -I/usr/include/libxml2 -I /usr/include/  -DHAVE_CONFIG_H 

LDFLAGS=$(ARCH) -shared -Wl,-soname,$(TARGETSO).$(MAJOR) -lxml2 -o 

SRCS= $(wildcard *.c) $(wildcard *.cpp)
OBJS=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SRCS)))

.SUFFIXES: .c .cpp 

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

.cpp.o:
	$(CXX) $(CXXFLAGS) -c -o $@ $<

.PHONY : all clean

all: $(TARGETAR) $(TARGETSO)

$(TARGETSO): $(OBJS)
	$(CXX) $(LDFLAGS) $(TARGETSO).$(VERSION) $(OBJS)
	$(LINK) $(TARGETSO).$(VERSION) $(TARGETSO).$(MAJOR)
	$(LINK) $(TARGETSO).$(MAJOR) $(TARGETSO)

clean:
	rm -f $(TARGETAR) $(TARGETSO) $(TARGETSO).$(VERSION) $(TARGETSO).$(MAJOR) $(OBJS) core
