#
# Makefile
#
# Circle - A C++ bare metal environment for Raspberry Pi
# Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

OBJS	= $(RASPPI)/actled.o $(RASPPI)/alloc.o $(RASPPI)/bcmmailbox.o $(RASPPI)/bcmpropertytags.o $(RASPPI)/gpiopin.o $(RASPPI)/gpiopinfiq.o $(RASPPI)/libhelper.o $(RASPPI)/libstub.o \
	  $(RASPPI)/memory.o $(RASPPI)/new.o $(RASPPI)/purecall.o $(RASPPI)/synchronize.o $(RASPPI)/sysinit.o $(RASPPI)/timer.o $(RASPPI)/util.o $(RASPPI)/util_fast.o \
	  $(RASPPI)/bcmframebuffer.o $(RASPPI)/device.o $(RASPPI)/koptions.o $(RASPPI)/pagetable.o $(RASPPI)/screen.o $(RASPPI)/nulldevice.o \
	  $(RASPPI)/assert.o $(RASPPI)/chargenerator.o $(RASPPI)/debug.o $(RASPPI)/devicenameservice.o $(RASPPI)/logger.o $(RASPPI)/serial.o $(RASPPI)/string.o \
	  $(RASPPI)/delayloop.o $(RASPPI)/exceptionhandler.o $(RASPPI)/exceptionstub.o $(RASPPI)/interrupt.o \
	  $(RASPPI)/gpiomanager.o $(RASPPI)/gpioclock.o $(RASPPI)/dmachannel.o $(RASPPI)/pwmsounddevice.o $(RASPPI)/pwmsoundbasedevice.o $(RASPPI)/pwmoutput.o $(RASPPI)/tracer.o \
	  $(RASPPI)/i2cmaster.o $(RASPPI)/i2cslave.o $(RASPPI)/spimaster.o $(RASPPI)/spimasterdma.o $(RASPPI)/virtualgpiopin.o $(RASPPI)/machineinfo.o \
	  $(RASPPI)/multicore.o $(RASPPI)/spinlock.o $(RASPPI)/bcmrandom.o $(RASPPI)/cache-v7.o $(RASPPI)/cputhrottle.o \
	  $(RASPPI)/ptrarray.o $(RASPPI)/ptrlist.o $(RASPPI)/time.o $(RASPPI)/libstub.o

ifeq ($(strip $(DEPTH)), 8)
LIBCIRCLE = libcircle8
else
LIBCIRCLE = libcircle
endif

all: $(RASPPI)/startup.o $(RASPPI)/$(LIBCIRCLE).a

$(RASPPI)/startup.o: startup.S
$(RASPPI)/libstub.o: libstub.S

$(RASPPI)/$(LIBCIRCLE).a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include ../Rules.mk
