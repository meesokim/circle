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

CIRCLEHOME = ../..

OBJS	= $(RASPPI)/dwhcidevice.o $(RASPPI)/dwhciregister.o $(RASPPI)/dwhcixferstagedata.o \
	  $(RASPPI)/usbconfigparser.o $(RASPPI)/usbdevice.o $(RASPPI)/usbendpoint.o $(RASPPI)/usbhostcontroller.o $(RASPPI)/usbrequest.o $(RASPPI)/usbstring.o \
	  $(RASPPI)/smsc951x.o $(RASPPI)/macaddress.o $(RASPPI)/netdevice.o $(RASPPI)/usbdevicefactory.o $(RASPPI)/usbstandardhub.o \
	  $(RASPPI)/usbmassdevice.o \
	  $(RASPPI)/dwhciframeschednper.o $(RASPPI)/dwhciframeschedper.o $(RASPPI)/usbkeyboard.o \
	  $(RASPPI)/dwhcirootport.o $(RASPPI)/usbhiddevice.o $(RASPPI)/usbmouse.o $(RASPPI)/usbgamepad.o \
	  $(RASPPI)/usbfunction.o \
	  $(RASPPI)/dwhciframeschednsplit.o $(RASPPI)/usbprinter.o $(RASPPI)/usbbluetooth.o $(RASPPI)/usbmidi.o $(RASPPI)/usbcdcethernet.o

$(RASPPI)/libusb.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
