all: usb dump

dump: dump.cpp
	g++ -lfuse -lserial -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=22 -Wall dump.cpp -o dump
usb: usb.cpp 
	g++ -lfuse -lserial -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=22 -Wall usb.cpp -o usb
clean:
	rm usb dump
