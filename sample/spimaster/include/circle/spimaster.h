//
// spimaster.h
//
// Supported features:
//	SPI0 device only
//	Standard mode (3-wire) only
//	Chip select lines (CE0, CE1) are active low
//	Polled operation only
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2015  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef _circle_spimaster_h
#define _circle_spimaster_h

#include <circle/gpiopin.h>
#include <circle/spinlock.h>
#include <circle/types.h>

class CSPIMaster
{
public:
	CSPIMaster (unsigned nClockSpeed = 500000, unsigned CPOL = 0, unsigned CPHA = 0);
	~CSPIMaster (void);

	boolean Initialize (void);

	// returns number of read bytes or < 0 on failure
	int Read (unsigned nChipSelect, void *pBuffer, unsigned nCount);

	// returns number of written bytes or < 0 on failure
	int Write (unsigned nChipSelect, const void *pBuffer, unsigned nCount);

	// returns number of bytes transfered or < 0 on failure
	int WriteRead (unsigned nChipSelect, const void *pWriteBuffer, void *pReadBuffer, unsigned nCount);

public:
	unsigned m_nClockSpeed;
	unsigned m_CPOL;
	unsigned m_CPHA;

	CGPIOPin m_SCLK;
	CGPIOPin m_MOSI;
	CGPIOPin m_MISO;
	CGPIOPin m_CE0;
	CGPIOPin m_CE1;

	CSpinLock m_SpinLock;
};

#endif
