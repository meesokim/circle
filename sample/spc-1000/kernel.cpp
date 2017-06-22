//
// kernel.cpp
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
#ifdef __cplusplus
extern "C" {
#endif
#include "Z80.h"
#include "common.h"
#include "mc6847.h"
#include "spcall.h"
#include "spckey.h"
#include "tms9918.h"
int printf(const char *format, ...);
long _sprintf(char *buf, char *format, ...);
char *xtoa( char *a, unsigned int x, int opt);
void SetPalette(int, int, int, int);
unsigned char *video_get_vbp(int i);
void tms9918_render_line(tms9918 vdp);
char vdp_buffer[256*192];
#ifdef __cplusplus
 }
#endif
#include "kernel.h"
#include "ay8910.h"
#include "casswindow.h"
#include <circle/string.h>
#include <circle/screen.h>
#include <circle/debug.h>
#include <circle/util.h>
#include <circle/bcmmailbox.h>
#include <assert.h>

//#define SOUND_SAMPLES		(sizeof Sound / sizeof Sound[0] / SOUND_CHANNELS)

#define PARTITION	"emmc1-1"
#define PARTITION0  "emmc1"
#define FILENAME	"circle.txt"

extern char tap0[];
char cas[1024*1024*5/8];
char tap[1024*1024*5];
extern char samsung_bmp_c[];
char *itoa( char *a, int i);
int sprintf(char *str, const char *format, ...);

typedef unsigned int uint32_t ;
typedef int int32_t;
typedef unsigned short uint16_t;

typedef enum
{
   VC_AUDIO_MSG_TYPE_RESULT,              // Generic result
   VC_AUDIO_MSG_TYPE_COMPLETE,              // playback of samples complete
   VC_AUDIO_MSG_TYPE_CONFIG,                 // Configure
   VC_AUDIO_MSG_TYPE_CONTROL,                 // control 
   VC_AUDIO_MSG_TYPE_OPEN,                 //  open
   VC_AUDIO_MSG_TYPE_CLOSE,                 // close/shutdown
   VC_AUDIO_MSG_TYPE_START,                 // start output (i.e. resume)
   VC_AUDIO_MSG_TYPE_STOP,                 // stop output (i.e. pause)
   VC_AUDIO_MSG_TYPE_WRITE,                 // write samples
   VC_AUDIO_MSG_TYPE_MAX

} VC_AUDIO_MSG_TYPE;

// configure the audio
typedef struct
{
   uint32_t channels;
   uint32_t samplerate;
   uint32_t bps;

} VC_AUDIO_CONFIG_T;

typedef struct
{
   uint32_t volume;
   uint32_t dest;

} VC_AUDIO_CONTROL_T;

// audio
typedef struct
{
   uint32_t dummy;

} VC_AUDIO_OPEN_T;

// audio
typedef struct
{
   uint32_t dummy;

} VC_AUDIO_CLOSE_T;
// audio
typedef struct
{
   uint32_t dummy;

} VC_AUDIO_START_T;
// audio
typedef struct
{
   uint32_t draining;

} VC_AUDIO_STOP_T;

// configure the write audio samples
typedef struct
{
   uint32_t count; // in bytes
   void *callback;
   void *cookie;
   uint16_t silence;
   uint16_t max_packet;
} VC_AUDIO_WRITE_T;

// Generic result for a request (VC->HOST)
typedef struct
{
   int32_t success;  // Success value

} VC_AUDIO_RESULT_T;

// Generic result for a request (VC->HOST)
typedef struct
{
   int32_t count;  // Success value
   void *callback;
   void *cookie;
} VC_AUDIO_COMPLETE_T;

typedef struct
{
   int32_t type;     // Message type (VC_AUDIO_MSG_TYPE)
   union
   {
	VC_AUDIO_CONFIG_T    config;
    VC_AUDIO_CONTROL_T   control;
	VC_AUDIO_OPEN_T  open;
	VC_AUDIO_CLOSE_T  close;
	VC_AUDIO_START_T  start;
	VC_AUDIO_STOP_T  stop;
	VC_AUDIO_WRITE_T  write;
	VC_AUDIO_RESULT_T result;
	VC_AUDIO_COMPLETE_T complete;
   } u;
} VC_AUDIO_MSG_T;

static VC_AUDIO_MSG_T audio;
static tms9918 vdp;

SPCSystem spcsys;

static int tappos;
static int tapidx = 0;

int CasRead(CassetteTape *cas);
enum colorNum {
	COLOR_BLACK, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE,
	COLOR_RED, COLOR_BUFF, COLOR_CYAN, COLOR_MAGENTA,
	COLOR_ORANGE, COLOR_CYANBLUE, COLOR_LGREEN, COLOR_DGREEN };

void memset(byte *p, int length, int b);
int bpp;
static const char FromKernel[] = "kernel";
TKeyMap spcKeyHash[0x200]; 
unsigned char keyMatrix[10];
static CKernel *s_pThis;   
static TDirentry taps[256];
static int files = 0;
static int tapsize = 0;

int check_tap_file(char *str)
{
	int len = strlen(str);
	if ((len > 4) && (str[len-1] == 'p' || str[len-1] == 'P')&&(str[len-2] == 'a' || str[len-2] == 'A')&&(str[len-3] == 't'||str[len-3] == 'T')&&(str[len-4]=='.'))
		return 1;
	if ((len > 4) && (str[len-1] == 's' || str[len-1] == 'S')&&(str[len-2] == 'a' || str[len-2] == 'A')&&(str[len-3] == 'c'||str[len-3] == 'C')&&(str[len-4]=='.'))
		return 2;
	return 0;
}

void SetPalette(int idx, int r, int g, int b)
{
	s_pThis->SetPalette(idx, r, g, b);
	
}

void CKernel::seletape()
{
	memset(spcsys.cas.title, 0, 256);
	strcat (spcsys.cas.title, itoa(spcsys.cas.title, tapidx));
	strcat (spcsys.cas.title, ". ");
	if (taps[tapidx].chLongTitle[0] != 0)
		strcat (spcsys.cas.title, taps[tapidx].chLongTitle);
	else
		strcat (spcsys.cas.title, taps[tapidx].chTitle);
	tappos = -1;
}

void CKernel::loadtape()
{
	int length = taps[tapidx].nSize;
	int fin = m_FileSystem.FileOpen(taps[tapidx].chTitle);
	char c = taps[tapidx].chTitle[strlen(taps[tapidx].chTitle)-1];
	if (fin > 0)
	{
		if (c =='P' || c == 'p') // tap file
		{
			m_FileSystem.FileRead(fin, tap+10, length);
			m_FileSystem.FileClose(fin);
			memset(tap, '0', 10);
			tap[taps[tapidx].nSize+10] = 0;	
			tapsize = strlen(tap);
		} else // cas file
		{
			m_FileSystem.FileRead(fin, cas, length);
			m_FileSystem.FileClose(fin);
			memset(tap, '0', 10);
			for(int i = 0; i < length; i++)
			{
				for(int j = 0; j < 8; j++)
					tap[10+i*8+j] = ((cas[i] & (0x80 >> j)) ? '1' : '0');
			}
			tap[length*8+10] = 0;
			tapsize = strlen(tap);
		}
	}
}
CKernel::CKernel (void)
:	m_Screen(320,240),
	m_Framebuffer(320, 240, true),
	m_Memory (TRUE),
	m_Timer(&m_Interrupt),
	m_Logger (LogPanic, &m_Timer),
	m_DWHCI (&m_Interrupt, &m_Timer),
	m_ShutdownMode (ShutdownNone)
   ,ay8910(&m_Timer)
   ,m_PWMSound (&m_Interrupt)
   ,m_EMMC (&m_Interrupt, &m_Timer, &m_ActLED)
  // ,m_GUI(&m_Screen)
  // ,m_PWMSoundDevice (&m_Interrupt)
{
	//m_PWMSoundDevice.CancelPlayback();
	m_ActLED.Blink (5);	// show we are alive
	s_pThis = this;
}

CKernel::~CKernel (void)
{
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		int c = 0; 	
		bOK = m_Screen.Initialize (); // BGRA
		m_Screen.SetPalette(c++, (u32)COLOR32(0x00, 0x00, 0x00, 0xff)); /* BLACK */
		m_Screen.SetPalette(c++, (u32)COLOR32(0x07, 0xff, 0x00, 0xff)); /* GREEN */ 
		m_Screen.SetPalette(c++, (u32)COLOR32(0xff, 0xff, 0x00, 0xff)); /* YELLOW */
		m_Screen.SetPalette(c++, (u32)COLOR32(0x3b, 0x08, 0xff, 0xff)); /* BLUE */
		m_Screen.SetPalette(c++, (u32)COLOR32(0xcc, 0x00, 0x3b, 0xff)); /* RED */
		m_Screen.SetPalette(c++, (u32)COLOR32(0xff, 0xff, 0xff, 0xff)); /* BUFF */
		m_Screen.SetPalette(c++, (u32)COLOR32(0x07, 0xe3, 0x99, 0xff)); /* CYAN */
		m_Screen.SetPalette(c++, (u32)COLOR32(0xff, 0x1c, 0xff, 0xff)); /* MAGENTA */
		m_Screen.SetPalette(c++, (u32)COLOR32(0xff, 0x81, 0x00, 0xff)); /* ORANGE */
		
		m_Screen.SetPalette(c++, (u32)COLOR32(0x07, 0xff, 0x00, 0xff)); /* GREEN */
		m_Screen.SetPalette(c++, (u32)COLOR32(0xff, 0xff, 0xff, 0xff)); /* BUFF */
		
		m_Screen.SetPalette(c++, (u32)COLOR32(0x00, 0x3f, 0x00, 0xff)); /* ALPHANUMERIC DARK GREEN */
		m_Screen.SetPalette(c++, (u32)COLOR32(0x07, 0xff, 0x00, 0xff)); /* ALPHANUMERIC BRIGHT GREEN */ 
		m_Screen.SetPalette(c++, (u32)COLOR32(0x91, 0x00, 0x00, 0xff)); /* ALPHANUMERIC DARK ORANGE */
		m_Screen.SetPalette(c++, (u32)COLOR32(0xff, 0x81, 0x00, 0xff)); /* ALPHANUMERIC BRIGHT ORANGE */		
		m_Screen.SetPalette(0xff,(u32)COLOR32(0xff, 0xff, 0xff, 0xff));
		m_Screen.SetPalette(0x46,(u32)COLOR32(0xff, 0x00, 0x00, 0xff));
		m_Screen.UpdatePalette();
	}
	m_Framebuffer.Initialize();
	m_EMMC.Initialize ();
	
	memcpy(m_Screen.GetBuffer(), samsung_bmp_c, 320*240);
	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}
	memcpy(tap, tap0, strlen(tap0));
	if (bOK)
	{
		CDevice *pTarget;
//		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
		}
		pTarget = &m_Screen;
		bOK = m_Logger.Initialize (pTarget);
//		spcsys.volue = m_Options.GetDecimal("volume");
		spcsys.volume = 10;
		if (spcsys.volume < 0)
			spcsys.volume = 8;
	}
	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}
	if (bOK)
	{
		bOK = m_DWHCI.Initialize ();
	}                       	
	int num = 0;

	do {
		spcKeyHash[spcKeyMap[num].sym] = spcKeyMap[num];
	} while(spcKeyMap[num++].sym != 0);
	reset();
	return bOK;
}

void CKernel::reset()
{
	memset(spcsys.VRAM, 0x2000, 0x0);
	memset(keyMatrix, 10, 0xff);
	bpp = m_Screen.GetDepth();
	spcsys.IPLK = 1;
	spcsys.GMODE = 0;
	spcsys.cas.motor = 0;
	spcsys.cas.button = CAS_PLAY;
	ay8910.Reset8910(&(spcsys.ay8910), 0);
	reset_flag = 1;
	spcsys.cas.lastTime = 0;
	spcsys.turbo = 0;
	vdp = tms9918_create();
	vdp->show = 0;
	return;
}

void CKernel::rotate(int i, int idx)
{
	//m_Screen.Rotor(i, idx);
}

int CKernel::dspcallback(u32 *stream, int len) 
{
	static unsigned int nCount = 0;
	ay8910.DSPCallBack(stream, len);
	
	for(int i = 0; i < len; i++)
	{
		stream[i] = stream[i] >> (10-spcsys.volume);
	}
//	memcpy(stream, (void *)(&Sound[0]+nCount), len);
//	m_Logger.Write (FromKernel, LogNoti*******ce, "Compile time: " __DATE__ " " __TIME__);
	return len;
}

void CKernel::toggle_turbo()
{
	spcsys.turbo = !spcsys.turbo;
}

void CKernel::volume(int i)
{
	if (i > 0 && spcsys.volume < 10)
		spcsys.volume++;
	if (i < 0 && spcsys.volume > 0)
		spcsys.volume--;
}

int count = 0;
#define WAITTIME 983
TShutdownMode CKernel::Run (void)
{
	int frame = 0, ticks = 0, pticks = 0, d = 0;
	unsigned int t = 0;
	int cycles = 0;	
	int time = 0, i;
	int *dst, *src;
	char str[100];
	int perc;
	tappos = 0;
	CDevice *pPartition = m_DeviceNameService.GetDevice (PARTITION, TRUE);
	if (pPartition == 0)
		pPartition = m_DeviceNameService.GetDevice (PARTITION0, TRUE);
	if (pPartition == 0)
	{
		m_Logger.Write (FromKernel, LogPanic, "Partition not found: %s", PARTITION);
	}

	if (!m_FileSystem.Mount (pPartition))
	{
		m_Logger.Write (FromKernel, LogPanic, "Cannot mount partition: %s", PARTITION);
	}	
	CUGUI m_GUI(&m_Framebuffer);
	m_GUI.Initialize();
	m_GUI.ShowMouse(false);
	UG_FontSelect(&FONT_8X12);
	tapsize = strlen(tap0) + 1000;
	CString Message;
	Uint8 fb[320*240];
	m_Framebuffer.SetBuffer(fb);
	m_Logger.Initialize (&m_Screen);
	TScreenStatus status = m_Screen.GetStatus();
	status.Color = 0xff;
	status.bCursorOn = 0;
	m_Screen.SetStatus(status);
	CBcmPropertyTags Tags;
	TPropertyTagVchiqInit vchiqInit;
	Tags.GetTag (PROPTAG_SET_VCHIQ_INIT, &vchiqInit, sizeof vchiqInit);		
#if 0	// vchiq audio control for sound path 3.5mm to HDMI
	CBcmMailBox m_mbSound(3);
	audio.type = VC_AUDIO_MSG_TYPE_OPEN;
	m_mbSound.WriteRead((unsigned int)&audio);
	audio.type = VC_AUDIO_MSG_TYPE_CONTROL;
	audio.u.control.volume = 0;
	audio.u.control.dest = 0;
	m_mbSound.WriteRead((unsigned int)&audio);		
#endif
	m_PWMSound.Play(this);//, SOUND_CHANNELS, SOUND_BITS,Sound, SOUND_SAMPLES );
	InitMC6847(fb, &spcsys.VRAM[0], 256,192);	
	
	//m_PWMSound.Playback (Sound, SOUND_SAMPLES, SOUND_CHANNELS, SOUND_BITS);
//	CCassWindow CassWindow (0, 0);	
	CUSBKeyboardDevice *pKeyboard = (CUSBKeyboardDevice *) m_DeviceNameService.GetDevice ("ukbd1", FALSE);
	if (pKeyboard == 0)
	{
		m_Logger.Write (FromKernel, LogError, "Keyboard not found");
	} 
	else
	{
		pKeyboard->RegisterKeyStatusHandlerRaw (KeyStatusHandlerRaw); 		
	}
	// Show contents of root directory
#if 1	
	TDirentry Direntry;
	TFindCurrentEntry CurrentEntry;
	
	unsigned nEntry = m_FileSystem.RootFindFirst (&Direntry, &CurrentEntry);
	for (unsigned i = 0; nEntry != 0; i++)
	{
		if (!(Direntry.nAttributes & FS_ATTRIB_SYSTEM))
		{
			CString FileName;
			int ret = check_tap_file(Direntry.chTitle);
			if (ret > 0)
				taps[files++] = Direntry;
		}

		nEntry = m_FileSystem.RootFindNext (&Direntry, &CurrentEntry);
	}
#endif
	Z80 *R = &spcsys.Z80R;	
	reset_flag = 1;
	while(1)
	{
		if (reset_flag) {
			ResetZ80(R);
			R->ICount = I_PERIOD;
			pticks = ticks = m_Timer.GetClockTicks();
			spcsys.cycles = 0;	
			reset_flag = 0;
		}
		count = R->ICount;
		ExecZ80(R); // Z-80 emulation
		spcsys.cycles += (count - R->ICount);
		if (R->ICount <= 0)
		{
			frame++;
			spcsys.tick++;		// advance spc tick by 1 msec
			R->ICount += I_PERIOD;	// re-init counter (including compensation)
			if (frame % 16 == 0)
			{
				if (R->IFF & IFF_EI)	// if interrupt enabled, call Z-80 interrupt routine
				{
					R->IFF |= IFF_IM1 | IFF_1;
					IntZ80(R, 0);
				}
			}
			if (frame%33 == 0)
			{
				if (vdp != 0 && vdp->show == 1)
				{
					char *p = (char *)m_Framebuffer.GetBuffer()+320*(240-192)/2;
					for (int i = 0; i < 192; i++)
					{
						vdp->scanline = i;
						tms9918_render_line(vdp);
						//memset(video_get_vbp(i), 34, 256);
						memcpy(p+(320-256)/2, video_get_vbp(i), 256);
						//memset(p+(320-256)/2, 34, 256);
						p += 320;
					}
				} else
					Update6847(spcsys.GMODE, m_Framebuffer.GetBuffer());
				if (spcsys.cas.read)
				{
					perc = tappos * 100 / tapsize;
					UG_FillFrame( 40, 10, 40+270*perc/100, 15, 0x3);
					UG_SetForecolor( 0xff);
					UG_SetBackcolor( 0x0);
					UG_PutString( 3, 7, title );
					spcsys.cas.title[0] = 0;
				}
				if (spcsys.cas.title)
				{
					UG_SetForecolor( 0xff);
					UG_SetBackcolor( 0x0);
					UG_PutString( 3, 220, spcsys.cas.title );
				}
				memcpy(m_Screen.GetBuffer(), m_Framebuffer.GetBuffer(), 320*240);
			}
			ay8910.Loop8910(&spcsys.ay8910, 1);
			ticks = m_Timer.GetClockTicks() - ticks;
			if (!spcsys.cas.read && !spcsys.turbo && ticks < WAITTIME)
				m_Timer.usDelay(WAITTIME - (ticks < WAITTIME ? ticks : WAITTIME));
			spcsys.cas.read = 0;
			//m_Timer.usDelay(ticks);
			ticks = m_Timer.GetClockTicks() - (ticks > WAITTIME ? ticks - WAITTIME : 0);
			if (frame%1000  == 0)
			{
				//printf ("Address: %04x)", R->PC);
				//s_pThis->printf("%d, %d\n", spcsys.cycles-cycles, m_Timer.GetClockTicks() - time);
				cycles = spcsys.cycles;
				time = m_Timer.GetClockTicks();
			}
		}
	}
	return ShutdownHalt;
}

void CKernel::KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6])
{
	CString Message;
	Message.Format ("Key status (modifiers %02X, %s)", (unsigned) ucModifiers, RawKeys);
	TKeyMap *map;
	memset(keyMatrix, 10, 0xff);
	if (ucModifiers != 0)
	{
		if ((ucModifiers & 0x10 || ucModifiers & 0x01) & (ucModifiers & 0x40 || ucModifiers & 0x4)) {
			if (RawKeys[0] == 0x4c)
				s_pThis->reset();
		}
		if ((ucModifiers == 0x8 || ucModifiers == 0x40)) {
			if (files > 0)
			{
				if (RawKeys[0] == 0x50) {
					tapidx -= 1;
					if (tapidx < 0)
						tapidx = files - 1;
					s_pThis->seletape();
					return;
				} else if (RawKeys[0] == 0x4f) {
					tapidx += 1;
					if (tapidx == files)
						tapidx = 0;
					s_pThis->seletape();
					return;
				}
			}
			if (RawKeys[0] == 0x44) {
				s_pThis->toggle_turbo();
				return;
			} else if (RawKeys[0] == 0x52) {
				s_pThis->volume(1);
				return;
			} else if (RawKeys[0] == 0x51) {
				s_pThis->volume(-1);
				return;
			} else if (RawKeys[0] == 0x2B) {
				if (vdp->show == 1) 
					vdp->show = -1;
				else if (vdp->show == -1)
					vdp->show = 1;
				return;
			}
		}
		for(int i = 0; i < 8; i++)
			if ((ucModifiers & (1 << i)) != 0)
			{
				map = &spcKeyHash[0x100 | (1 << i)];
				if (map != 0)
					keyMatrix[map->keyMatIdx] &= ~ map->keyMask;
			}
	}

	for (unsigned i = 0; i < 6; i++)
	{
		if (RawKeys[i] != 0)
		{
			map = &spcKeyHash[RawKeys[i]];
			if (map != 0)
				keyMatrix[map->keyMatIdx] &= ~ map->keyMask;
		}
	}
//	printf(Message);
}

int CKernel::printf(const char *format, ...)
{
	CString Message;
	va_list args;
    va_start(args, format);
	Message.FormatV(format, args);
	s_pThis->m_Logger.Write (Message);
    va_end(args);	
	return 0;
}


int ReadVal(void) 
{
	char c = 0;
	if (tappos < 0)
	{
		s_pThis->loadtape();
		tappos = 0;
	}
	c = tap[tappos++];
	spcsys.cas.read++;
	if (tappos > tapsize)
		tappos = 0;
//	sprintf(s_pThis->title, "%d %c\r", tappos, c);
	return (c == '1' ? 1 : 0);
}

void OutZ80(register word Port,register byte Value)
{
	printf("VRAM[%x]=%x\n", Port, Value);

	if ((Port & 0xE000) == 0x0000) // VRAM area
	{
		spcsys.VRAM[Port] = Value;
	}
	else if ((Port & 0xE000) == 0xA000) // IPLK area
	{
		spcsys.IPLK = !spcsys.IPLK;	// flip IPLK switch
	}
	else if ((Port & 0xE000) == 0x2000)	// GMODE setting
	{
		spcsys.GMODE = Value;
#ifdef DEBUG_MODE
		printf("GMode:%02X\n", Value);
#endif
	}
	else if ((Port & 0xD860) == 0xD860)
	{
		if (Port & 1)
			tms9918_writeport1(vdp, Value);
		else
			tms9918_writeport0(vdp, Value);
			
	}
	else if ((Port & 0xE000) == 0x6000) // SMODE
	{
		if (spcsys.cas.button != CAS_STOP)
		{

			if ((Value & 0x02)) // Motor
			{
				if (spcsys.cas.pulse == 0)
				{
					spcsys.cas.pulse = 1;

				}
			}
			else
			{
				if (spcsys.cas.pulse)
				{
					spcsys.cas.pulse = 0;
					if (spcsys.cas.motor)
					{
						spcsys.cas.motor = 0;
					}
					else
					{
						spcsys.cas.motor = 1;
					}
				}
			}
		}

//		if (spcsys.cas.button == CAS_REC && spcsys.cas.motor)
//		{
//			CasWrite(&spcsys.cas, Value & 0x01);
//		}
	}
	else if ((Port & 0xFFFE) == 0x4000) // PSG
	{

		if (Port & 0x01) // Data
		{
		    if (spcsys.psgRegNum == 15) // Line Printer
			{
				if (Value != 0)
				{
			    //spcsys.prt.bufs[spcsys.prt.length++] = Value;
				//                    printf("PRT <- %c (%d)\n", Value, Value);
				//                    printf("%s(%d)\n", spcsys.prt.bufs, spcsys.prt.length);
				}
			}
			s_pThis->ay8910.Write8910(&spcsys.ay8910, (byte) spcsys.psgRegNum, Value);
		}
		else // Reg Num
		{
			spcsys.psgRegNum = Value;
			s_pThis->ay8910.WrCtrl8910(&spcsys.ay8910, Value);
		}
	}	
}

byte InZ80(register word Port)
{
	if (Port >= 0x8000 && Port <= 0x8009) // Keyboard Matrix
	{
		return keyMatrix[Port&0xf];
	}
	else if ((Port & 0xE000) == 0xA000) // IPLK
	{
		spcsys.IPLK = !spcsys.IPLK;
	} else if ((Port & 0xE000) == 0x2000) // GMODE
	{
		return spcsys.GMODE;
	} else if ((Port & 0xE000) == 0x0000) // VRAM reading
	{
		return spcsys.VRAM[Port];
	} else if ((Port & 0xD860) == 0xD860)
	{
		if (Port & 1)
			return tms9918_readport1(vdp);
		else
			return tms9918_readport0(vdp);
	}	else if ((Port & 0xFFFE) == 0x4000) // PSG
	{
		byte retval = 0x1f;
		if (Port & 0x01) // Data
		{
			if (spcsys.psgRegNum == 14)
			{
				// 0x80 - cassette data input
				// 0x40 - motor status
				// 0x20 - print status
//				if (spcsys.prt.poweron)
//                {
//                    printf("Print Ready Check.\n");
//                    retval &= 0xcf;
//                }
//                else
//                {
//                    retval |= 0x20;
//                }
				if (spcsys.cas.motor)
				{
					if (CasRead(&spcsys.cas) == 1)
							retval |= 0x80; // high
						else
							retval &= 0x7f; // low
				}
				if (spcsys.cas.motor)
					retval &= (~(0x40)); // 0 indicates Motor On
				else
					retval |= 0x40;

			}
			else 
			{
				int data = s_pThis->ay8910.RdData8910(&spcsys.ay8910);
				//printf("r(%d,%d)\n", spcsys.psgRegNum, data);
				return data;
			}
		} else if (Port & 0x02)
		{
            retval = (ReadVal() == 1 ? retval | 0x80 : retval & 0x7f);
		}
		return retval;
	}
	return 0;
}

//#define STONE (21/48000*4000000) // 21 sample * 48000Hz 1792
#define STONE (57*32) // 21 sample * 48000Hz 1792
#define LTONE (STONE*2)

int CasRead(CassetteTape *cas)
{
	int cycles;
	int bitTime;
	
	cycles = spcsys.cycles + (count - spcsys.Z80R.ICount);
	bitTime = cycles - cas->lastTime;

	if (bitTime >= cas->bitTime)
	{
		cas->rdVal = ReadVal();
		cas->lastTime = cycles;
		cas->bitTime = cas->rdVal ? (LTONE * 0.6) : (STONE * 0.9);
		sprintf(s_pThis->title, "%d%\r", tappos * 100 / tapsize);		
	}

	switch (cas->rdVal)
	{
	case 0:
		if (bitTime < STONE/2)
			return 1; // high
		else
			return 0; // low

	case 1:
		if (bitTime < STONE)
			return 1; // high
		else
			return 0; // low
	}
	return 0; // low for other cases
}


void CasWrite(CassetteTape *cas, int val)
{
	Uint32 curTime;
	int t;

	t = (spcsys.cycles - cas->lastTime) >> 5;
	if (t > 100)
		cas->cnt0 = cas->cnt1 = 0;
	cas->lastTime = spcsys.cycles;
	if (cas->wrVal == 1)
	{
		if (val == 0)
			if (t > STONE/2) 
			{
//				printf("1");
//				cas->cnt0 = 0;
//				fputc('1', spconf.wfp);
			} else {
				if (cas->cnt0++ < 100)
				{
//					printf("0");
//					fputc('0', spconf.wfp);
				}
			}
	}
	cas->wrVal = val;
}

word LoopZ80(register Z80 *R) {
	return 0;
}

void PatchZ80(register Z80 *R) {
	return;
}

int sprintf(char *str, const char *format, ...)
{
	va_list args;
	va_start(args,format);
	CString s;
	s.FormatV(format, args);
	va_end(args);
	strcpy(str, s);
	//m_Logger.Write (str);
	return 1;
}

int printf(const char *format, ...)
{
	va_list args;
	va_start(args,format);
	CString s;
	s.FormatV(format, args);
	va_end(args);
	//s_pThis->m_Logger.Write (s);
	return 1;
}
void memset(byte *p, int length, int b)
{
	for (int i=0; i<length; i++)
		*p++ = b;
}


/*+-------------------------------------------------------------------------+
  |  FILE: sprintf.c                                                        |
  |  Version: 0.1                                                           |
  |                                                                         |
  |  Copyright (c) 2003 Chun Joon Sung                                      |
  |  Email: chunjoonsung@hanmail.net                                        |
  +-------------------------------------------------------------------------+*/
char *itoa( char *a, int i)
{
	int sign=0;
	int temp=0;
	char buf[16];
	char *ptr;

	ptr = buf;

	/* zero then return */
	if( i )
	{
		/* make string in reverse form */
		if( i < 0 )
			{ i = ~i + 1; sign++; }
		while( i )
			{ *ptr++ = (i % 10) + '0'; i = i / 10; }
		if( sign )
			*ptr++ = '-';
		*ptr = '\0';

		/* copy reverse order */
		for( i=0; i < strlen(buf); i++ )
			*a++ = buf[strlen(buf)-i-1];
	}	
	else
		*a++ = '0';

	return a;
}

char *xtoa( char *a, unsigned int x, int opt)
{
	int i;
	int sign=0;
	int temp=0;
	char buf[16];
	char *ptr;

	ptr = buf;

	/* zero then return */
	if( x )
	{
		/* make string in reverse form */
		while( x )
			{ *ptr++ = (x&0x0f)<10 ? (x&0x0f)+'0' : (x&0x0f)-10+opt; x>>= 4; }

		*ptr = '\0';

		/* copy reverse order */
		for( i=0; i < strlen(buf); i++ )
			*a++ = buf[strlen(buf)-i-1];
	}
	else	
		*a++ = '0';

	return a;
}

long _sprintf(char *buf, char *format, ...)
{
	int cont_flag;
	int value;
	int quit;
	va_list args;	
	va_start(args,format);	
	char *start=buf;
	long *argp=(long *)&args;
	char *p;

    while( *format )
	{
		if( *format != '%' )	/* 일반적인 문자 */
		{
			*buf++ = *format++;
			continue;
		}

		format++;				/* skip '%' */

		if( *format == '%' )	/* '%' 문자가 연속 두번 있는 경우 */
		{
			*buf++ = *format++;
			continue;
		}
		
		switch( *format )
		{
			case 'c' :
				*buf++ = *(char *)argp++;
				break;

			case 'd' :
				buf = itoa(buf,*(int *)argp++);
				break;

			case 'x' : 
				buf = xtoa(buf,*(unsigned int *)argp++,'a');
				break;

			case 'X' : 
				buf = xtoa(buf,*(unsigned int *)argp++,'A');
				break;

			case 's' :
				p=*(char **)argp++;
				while(*p) 
						*buf++ = *p++;
				break;

			default :
				*buf++ = *format; /* % 뒤에 엉뚱한 문자인 경우 */
				break;
        }

		format++;
    }
    *buf = '\0';

    return(buf-start);
}

