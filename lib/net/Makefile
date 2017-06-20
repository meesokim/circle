#
# Makefile
#
# Circle - A C++ bare metal environment for Raspberry Pi
# Copyright (C) 2015-2016  R. Stange <rsta2@o2online.de>
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


OBJS	= $(RASPPI)/netsubsystem.o $(RASPPI)/nettask.o $(RASPPI)/socket.o \
	  $(RASPPI)/transportlayer.o $(RASPPI)/networklayer.o $(RASPPI)/linklayer.o $(RASPPI)/netdevlayer.o $(RASPPI)/arphandler.o \
	  $(RASPPI)/icmphandler.o $(RASPPI)/routecache.o \
	  $(RASPPI)/netconnection.o $(RASPPI)/udpconnection.o \
	  $(RASPPI)/tcpconnection.o $(RASPPI)/retransmissionqueue.o $(RASPPI)/retranstimeoutcalc.o $(RASPPI)/tcprejector.o \
	  $(RASPPI)/netconfig.o $(RASPPI)/ipaddress.o $(RASPPI)/netqueue.o $(RASPPI)/checksumcalculator.o \
	  $(RASPPI)/dnsclient.o $(RASPPI)/ntpclient.o \
	  $(RASPPI)/dhcpclient.o $(RASPPI)/ntpdaemon.o $(RASPPI)/httpdaemon.o $(RASPPI)/tftpdaemon.o

$(RASPPI)/libnet.a: $(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS)

include $(CIRCLEHOME)/Rules.mk
