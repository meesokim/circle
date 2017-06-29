#
# Makefile
#
#

#ifeq ($(strip $(DEPTH)), 8)
#LIBCIRCLE = libcircle8
#else
#LIBCIRCLE = libcircle
#endif

CIRCLEHOME = ../..
INCLUDE = -I$(CIRCLEHOME)/addon

#OBJS	= main.o kernel.o
OBJS =  $(RASPPI)/main.o $(RASPPI)/kernel.o $(RASPPI)/Z80SPC.o $(RASPPI)/spcall.o \
		$(RASPPI)/MC6847.o $(RASPPI)/AY8910.o $(RASPPI)/pwmsound.o $(RASPPI)/tap.o \
		samsung.bmp.o $(RASPPI)/tms9918.o

LIBS =$(CIRCLEHOME)/addon/ugui/$(RASPPI)/libugui.a \
	  $(CIRCLEHOME)/addon/SDCard/$(RASPPI)/libsdcard.a \
	  $(CIRCLEHOME)/lib/usb/$(RASPPI)/libusb.a \
	  $(CIRCLEHOME)/lib/input/$(RASPPI)/libinput.a \
	  $(CIRCLEHOME)/lib/fs/$(RASPPI)/libfs.a \
	  $(CIRCLEHOME)/lib/fs/fat/$(RASPPI)/libfatfs.a \
	  $(CIRCLEHOME)/lib/fs/$(RASPPI)/libfs.a \
	  $(CIRCLEHOME)/lib/$(RASPPI)/$(LIBCIRCLE).a 

EXTRACLEAN = sound.h samsung.bmp.c tap.c

BMP = samsung

include ../Rules.mk

$(BMP).bmp.o: $(BMP).bmp
	bmp2bin $(BMP).bmp
	$(CC) $(CFLAGS) -c -o $@ $(BMP).bmp.c

sound.h: sound.h.gz
	gunzip -c sound.h.gz > sound.h

$(RASPPI)/tap.o: $(TAP_FILE)
	tap2c $(TAP_FILE) > tap.c
	$(CC) $(CFLAGS) -c -o $@ tap.c

%.bin: %.asm
	z80asm/z80asm -fb -l -sa $< 
spcall.c: spc-1000_all.bin spc-1000_ipl.bin
	cat spc-1000_all.bin spc-1000_ipl.bin > spcall.rom
	python bin2c.py spcall.rom ROM

bootcode: bootcode.bin start.elf fixup.dat
	wget https://github.com/raspberrypi/firmware/blob/master/boot/$<?raw=true -O $<	
