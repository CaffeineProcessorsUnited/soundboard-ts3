.PHONY : all clean dist

CXX=g++
CFLAGS=-c -O2 -Wall -fPIC -DLINUX -std=c++11
LD=g++
LFLAGS=
INCLUDE=./include/
LIBS=
PROGRAMS=cpusb
PREREQUISITES_=cpusb64 cpusb32
PREREQUISITES_64=cpusb64.o
PREREQUISITES_32=cpusb32.o
ARCH=64

cpusb32: CFLAGS+=-m32
cpusb32: LFLAGS+=-m32
cpusb32: ARCH=32

all: clean cpusb32 cpusb64

.SECONDEXPANSION:
cpusb%: $$(PREREQUISITES_$$(*))
	@echo "Linking library for $(ARCH)..."
	@mkdir -p dist
	$(LD) $(LFLAGS) $(LIBS) -o dist/cpusb$(ARCH).so -shared build/cpusb$(ARCH).o

cpusb%.o:
	@echo "Compiling library $* for $(ARCH)..."
	@mkdir -p build
	$(CXX) $(CFLAGS) -I$(INCLUDE) src/cpusb.cpp -o build/cpusb$(ARCH).o

clean:
	@echo "Cleaning up..."
	@rm -fr build/* dist/*

# Obviously only for testing reasons
dist:
	@echo "Copying all plugins to teamspeak folders"
	@if [ -f dist/cpusb64.so ]; then \
		rm -f /home/malte/Downloads/TeamSpeak3-Client-linux_amd64/plugins/cpusb.so;\
		cp dist/cpusb64.so /home/malte/Downloads/TeamSpeak3-Client-linux_amd64/plugins/cpusb.so;\
	fi
	@if [ -f dist/cpusb32.so ]; then \
		rm -f /home/malte/Downloads/TeamSpeak3-Client-linux_x86/plugins/cpusb.so;\
		cp dist/cpusb32.so /home/malte/Downloads/TeamSpeak3-Client-linux_x86/plugins/cpusb.so;\
	fi
