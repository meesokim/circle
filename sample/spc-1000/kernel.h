//
// kernel.h
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014  R. Stange <rsta2@o2online.de>
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
#ifndef _kernel_h
#define _kernel_h

class CKernel;

#include <circle/memory.h>
#include <circle/actled.h>
#include <circle/koptions.h>
#include <circle/devicenameservice.h>
#include <circle/screen.h>
#include <circle/serial.h>
#include <circle/exceptionhandler.h>
#include <circle/interrupt.h>
#include <circle/timer.h>
#include <circle/logger.h>
#include <circle/types.h>
#include <circle/pwmsounddevice.h>
#include <circle/usb/usbkeyboard.h>   
#include <circle/usb/dwhcidevice.h>
#include <circle/fs/fat/fatfs.h>
#include <ugui/uguicpp.h>
#include <sdcard/emmc.h>
#include "pwmsound.h"
#include "ay8910.h"

enum TShutdownMode
{
	ShutdownNone,
	ShutdownHalt,
	ShutdownReboot
};

class CKernel
{
public:
	CKernel (void);
	~CKernel (void);

	boolean Initialize (void);

	TShutdownMode Run (void);

private:
	// do not change this order
	CMemorySystem		m_Memory;
	CActLED				m_ActLED;
	CKernelOptions		m_Options;
	CDeviceNameService	m_DeviceNameService;
	CScreenDevice		m_Screen;
	CScreenDevice		m_Framebuffer;
	CSerialDevice		m_Serial;
	CExceptionHandler	m_ExceptionHandler;
	CInterruptSystem	m_Interrupt;
	CTimer				m_Timer;
	CLogger				m_Logger;
	CEMMCDevice			m_EMMC;
	CDWHCIDevice		m_DWHCI;
	CPWMSound			m_PWMSound; 
	CFATFileSystem		m_FileSystem;
	
//	CPWMSoundDevice		m_PWMSoundDevice;
	//void OutZ80(register word Port,register byte Value);
	//byte InZ80(register word Port);	
//	CUGUI				m_GUI;
	volatile TShutdownMode m_ShutdownMode;	
	int 				reset_flag;
	void reset();
	//Uint8 framebuffer[320*240*2];
public:
	CAY8910				ay8910;
	char title[1024];
	int dspcallback(unsigned *stream, int len);
	void rotate(int i, int idx);
	static void ShutdownHandler (void);
	static void KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6]);
	void SetPalette(int idx, int r, int g, int b) {
		m_Screen.SetPalette(idx, (u32)COLOR32(r, g, b, 0xff));
		m_Screen.UpdatePalette();
	}
	//static CKernel *s_pThis;
	static int printf(const char *format, ...);
	void seletape();
	void loadtape();
	void toggle_turbo();
	void volume(int i);
};

#endif
