#
# Makefile
#

CIRCLEHOME = ../../..

OBJS	= main.o kernel.o

LIBS	= ../$(RASPPI)/libsdcard.a \
	  $(CIRCLEHOME)/lib/fs/fat/$(RASPPI)/libfatfs.a \
	  $(CIRCLEHOME)/lib/fs/$(RASPPI)/libfs.a \
	  $(CIRCLEHOME)/lib/$(RASPPI)/$(LIBCIRCLE).a 

include $(CIRCLEHOME)/sample/Rules.mk
