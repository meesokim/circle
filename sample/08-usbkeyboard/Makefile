#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= $(RASPPI)/main.o $(RASPPI)/kernel.o

LIBS	= $(CIRCLEHOME)/lib/usb/$(RASPPI)/libusb.a \
	  $(CIRCLEHOME)/lib/input/$(RASPPI)/libinput.a \
	  $(CIRCLEHOME)/lib/fs/$(RASPPI)/libfs.a \
	  $(CIRCLEHOME)/lib/$(RASPPI)/$(LIBCIRCLE).a 

include ../Rules.mk
