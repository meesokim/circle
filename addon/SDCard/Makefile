#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= $(RASPPI)/emmc.o

$(RASPPI)/libsdcard.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
