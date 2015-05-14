# top level makefile
MKDIR    = mkdir -p
INSTALL = /usr/bin/install -c 
LINK = ln -s
INSTALL_DIR = /usr/local

all: 
	for dir in libevt libfpm build; do \
		cd $$dir && make && cd ..; \
	done

clean:
	for dir in libevt libfpm build; do \
		cd $$dir && make clean && cd ..; \
	done

install:  
	$(INSTALL) libevt/libevt.* 	$(INSTALL_DIR)/lib
	$(INSTALL) libfpm/libfpm.* 	$(INSTALL_DIR)/lib

