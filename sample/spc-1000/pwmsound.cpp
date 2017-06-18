//
// pwmsound.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
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
#include "kernel.h"
#include "pwmsound.h"
#include <assert.h>

// Playback works with 44100 Hz, Stereo, 12 Bit only.
// Other sound formats will be converted on the fly.

#define SAMPLE_RATE		44100

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

void DSPCallBack(void* unused, unsigned char *stream, int len);

CPWMSound::CPWMSound(CInterruptSystem *pInterrupt)
:	CPWMSoundBaseDevice (pInterrupt, SAMPLE_RATE),
	m_pSoundData (0),
	m_mbSound(3)	
{
	assert ((1 << 12) <= GetRange () && GetRange () < (1 << 13));	// 12 bit range
	#if 0
	CBcmPropertyTags Tags;
	TPropertyTagVchiqInit vchiqInit;
	Tags.GetTag (PROPTAG_GET_TURBO, &vchiqInit, sizeof vchiqInit);
	#endif	
	#if 0
	audio.type = VC_AUDIO_MSG_TYPE_OPEN;
	m_mbSound.WriteRead((unsigned int)&audio);
	audio.type = VC_AUDIO_MSG_TYPE_CONTROL;
	audio.u.control.volume = 0;
	audio.u.control.dest = 0;
	m_mbSound.WriteRead((unsigned int)&audio);	
	#endif
}

CPWMSound::~CPWMSound(void)
{
}
 
void CPWMSound::Playback (void *pSoundData, unsigned nSamples, unsigned nChannels, unsigned nBitsPerSample)
{
	assert (!IsActive ());

	assert (pSoundData != 0);
	assert (nChannels == 1 || nChannels == 2);
	assert (nBitsPerSample == 8 || nBitsPerSample == 16);

	m_p = m_pSoundData	 = (u8 *) pSoundData;
	m_s = m_nSamples	 = nSamples;
	m_nChannels	 = nChannels;
	m_nBitsPerSample = nBitsPerSample;

	Start ();
}

boolean CPWMSound::PlaybackActive (void) const
{
	return IsActive ();
}

void CPWMSound::CancelPlayback (void)
{
	Cancel ();
}

unsigned CPWMSound::GetChunk (u32 *pBuffer, unsigned nChunkSize)
{
	assert (pBuffer != 0);
	assert (nChunkSize > 0);
	assert ((nChunkSize & 1) == 0);

	unsigned nResult = 0;

	if (1)
	{
		m_kernel->dspcallback(pBuffer, nChunkSize);
		return nChunkSize;
		//m_pSoundData	 = (u8 *) m_p;
		//m_nSamples	 = m_s;
	}

	assert (m_pSoundData != 0);
	assert (m_nChannels == 1 || m_nChannels == 2);
	assert (m_nBitsPerSample == 8 || m_nBitsPerSample == 16);

	for (unsigned nSample = 0; nSample < nChunkSize / 2;)		// 2 channels on output
	{
		unsigned nValue = *m_pSoundData++;
		if (m_nBitsPerSample > 8)
		{
			nValue |= (unsigned) *m_pSoundData++ << 8;
			nValue = (nValue + 0x8000) & 0xFFFF;		// signed -> unsigned (16 bit)
		}
		
		if (m_nBitsPerSample >= 12)
		{
			nValue >>= m_nBitsPerSample - 12;
		}
		else
		{
			nValue <<= 12 - m_nBitsPerSample;
		}

		pBuffer[nSample++] = nValue;
	
		if (m_nChannels == 2)
		{
			nValue = *m_pSoundData++;
			if (m_nBitsPerSample > 8)
			{
				nValue |= (unsigned) *m_pSoundData++ << 8;
				nValue = (nValue + 0x8000) & 0xFFFF;	// signed -> unsigned (16 bit)
			}

			if (m_nBitsPerSample >= 12)
			{
				nValue >>= m_nBitsPerSample - 12;
			}
			else
			{
				nValue <<= 12 - m_nBitsPerSample;
			}
		}

		pBuffer[nSample++] = nValue;

		nResult += 2;

		if (--m_nSamples == 0)
		{
			break;
		}
	}
	
	return nResult;
}
