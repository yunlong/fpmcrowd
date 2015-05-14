TARGET=fpmcrowd
CC=gcc -O2 -g 
CXX=g++ -O2 -g 
CFLAGS= -DNDEBUG -D_UNIX -D_LINUX -D_REENTRANT -Wall -fPIC  -I. -I/usr/include/libxml2 -I/usr/include/ -DHAVE_CONFIG_H -I../libevt
all: $(TARGET) 
fpmcrowd: main.cpp fastcgi.cpp
	$(CXX) $(CFLAGS) -o fpmcrowd main.cpp fastcgi.cpp -I../libfpm -L../libfpm -lfpm -L../libevt/ -levt -lxml2 
clean:
	rm -f fpmcrowd  *.o 




