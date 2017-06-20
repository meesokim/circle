#
# Makefile
#

CIRCLEHOME = ../..

OBJS	= $(RASPPI)/ugui.o $(RASPPI)/uguicpp.o

$(RASPPI)/libugui.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
