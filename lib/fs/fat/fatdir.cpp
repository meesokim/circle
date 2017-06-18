//
// fatdir.cpp
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
#include <circle/fs/fat/fatdir.h>
#include <circle/timer.h>
#include <circle/util.h>
#include <assert.h>

typedef char BYTE;
typedef double DWORD;
typedef unsigned short WCHAR;

typedef struct _LongFileName
{
   BYTE sequenceNo;            // Sequence number, 0xe5 for
                               // deleted entry
   BYTE fileName_Part1[10];    // file name part
   BYTE fileattribute;         // File attibute
   BYTE reserved_1;
   BYTE checksum;              // Checksum
   BYTE fileName_Part2[12];    // WORD reserved_2;
   BYTE reserved_2[2];
   BYTE fileName_Part3[4];
}LFN;

typedef struct _FileMarker_Part2
{
   DWORD _Mark1;
   DWORD _Mark2;
   DWORD _Mark3;
}FMark;	

void wstrcpy(char *dst, char *src, int len)
{
	while(len--) {
		*dst++ = *src++;
		src++;
	}
}

#if 0
void strcat(char *dst, char *src)
{
	while(*dst++!=0);
	while(*src!=0)
		*dst++ = *src++;
}
#endif

CFATDirectory::CFATDirectory (CFATCache *pCache, CFATInfo *pFATInfo, CFAT *pFAT)
:	m_pCache (pCache),
	m_pFATInfo (pFATInfo),
	m_pFAT (pFAT),
	m_pBuffer (0),
	m_Lock (TASK_LEVEL)
{
}

CFATDirectory::~CFATDirectory (void)
{
	m_pCache = 0;
	m_pFATInfo = 0;
	m_pFAT = 0;
}

TFATDirectoryEntry *CFATDirectory::GetEntry (const char *pName)
{
	assert (pName != 0);

	char FATName[FAT_DIR_NAME_LENGTH];
	if (!Name2FAT (pName, FATName))
	{
		return 0;
	}
	
	assert (m_pFATInfo != 0);
	TFATType FATType = m_pFATInfo->GetFATType ();

	unsigned nEntry = 0;

	unsigned nEntriesPerCluster = 0;
	unsigned nCluster = 0;
	if (FATType == FAT32)
	{
		nCluster = m_pFATInfo->GetRootCluster ();

		nEntriesPerCluster =   m_pFATInfo->GetSectorsPerCluster ()
				     * FAT_DIR_ENTRIES_PER_SECTOR;
	}

	m_Lock.Acquire ();
	
	while (1)
	{
		if (FATType == FAT16)
		{
			if (nEntry >= m_pFATInfo->GetRootEntries ())
			{
				break;
			}
		}
		else
		{
			assert (FATType == FAT32);
			if (m_pFAT->IsEOC (nCluster))
			{
				break;
			}
		}
		
		unsigned nSector;
		if (FATType == FAT16)
		{
			nSector =   m_pFATInfo->GetFirstRootSector ()
				  + nEntry / FAT_DIR_ENTRIES_PER_SECTOR;
		}
		else
		{
			assert (FATType == FAT32);
			nSector =   m_pFATInfo->GetFirstSector (nCluster)
				  +   (nEntry % nEntriesPerCluster)
				    / FAT_DIR_ENTRIES_PER_SECTOR;
		}

		assert (m_pBuffer == 0);
		assert (m_pCache != 0);
		m_pBuffer = m_pCache->GetSector (nSector, 0);
		assert (m_pBuffer != 0);

		unsigned nOffset = (nEntry * FAT_DIR_ENTRY_SIZE) % FAT_SECTOR_SIZE;
		TFATDirectoryEntry *pFATEntry = (TFATDirectoryEntry *) &m_pBuffer->Data[nOffset];
		assert (pFATEntry != 0);

		if (pFATEntry->Name[0] == FAT_DIR_NAME0_LAST)
		{
			m_pCache->FreeSector (m_pBuffer, 1);
			m_pBuffer = 0;

			break;
		}

		if (   pFATEntry->Name[0] != FAT_DIR_NAME0_FREE
		    && !(pFATEntry->nAttributes & (FAT_DIR_ATTR_VOLUME_ID | FAT_DIR_ATTR_DIRECTORY))
		    && memcmp (pFATEntry->Name, FATName, FAT_DIR_NAME_LENGTH) == 0)
		{
			return pFATEntry;
		}

		m_pCache->FreeSector (m_pBuffer, 1);
		m_pBuffer = 0;

		nEntry++;

		if (   FATType == FAT32
		    && nEntry % nEntriesPerCluster == 0)
		{
			assert (m_pFAT != 0);
			nCluster = m_pFAT->GetClusterEntry (nCluster);
		}
	}

	m_Lock.Release ();

	return 0;
}

TFATDirectoryEntry *CFATDirectory::CreateEntry (const char *pName)
{
	assert (pName != 0);

	char FATName[FAT_DIR_NAME_LENGTH];
	if (!Name2FAT (pName, FATName))
	{
		return 0;
	}
	
	assert (m_pFATInfo != 0);
	TFATType FATType = m_pFATInfo->GetFATType ();

	unsigned nEntry = 0;

	unsigned nEntriesPerCluster = 0;
	unsigned nCluster = 0;
	if (FATType == FAT32)
	{
		nCluster = m_pFATInfo->GetRootCluster ();

		nEntriesPerCluster =   m_pFATInfo->GetSectorsPerCluster ()
				     * FAT_DIR_ENTRIES_PER_SECTOR;
	}

	m_Lock.Acquire ();

	unsigned nPrevCluster = 0;
	
	while (1)
	{
		if (FATType == FAT16)
		{
			if (nEntry >= m_pFATInfo->GetRootEntries ())
			{
				break;
			}
		}
		else
		{
			assert (FATType == FAT32);

			assert (m_pFAT != 0);
			if (m_pFAT->IsEOC (nCluster))
			{
				nCluster = m_pFAT->AllocateCluster ();
				if (nCluster == 0)
				{
					break;
				}

				unsigned nSector = m_pFATInfo->GetFirstSector (nCluster);
				for (unsigned i = 0; i < m_pFATInfo->GetSectorsPerCluster (); i++)
				{
					assert (m_pCache != 0);
					TFATBuffer *pBuffer = m_pCache->GetSector (nSector++, 1);
					assert (pBuffer != 0);

					memset (pBuffer->Data, 0, FAT_SECTOR_SIZE);

					m_pCache->MarkDirty (pBuffer);
					m_pCache->FreeSector (pBuffer, 1);
				}

				assert (nPrevCluster >= 2);
				m_pFAT->SetClusterEntry (nPrevCluster, nCluster);
				nPrevCluster = 0;
			}
		}
		
		unsigned nSector;
		if (FATType == FAT16)
		{
			nSector =   m_pFATInfo->GetFirstRootSector ()
				  + nEntry / FAT_DIR_ENTRIES_PER_SECTOR;
		}
		else
		{
			assert (FATType == FAT32);
			nSector =   m_pFATInfo->GetFirstSector (nCluster)
				  +   (nEntry % nEntriesPerCluster)
				    / FAT_DIR_ENTRIES_PER_SECTOR;
		}

		assert (m_pBuffer == 0);
		assert (m_pCache != 0);
		m_pBuffer = m_pCache->GetSector (nSector, 0);
		assert (m_pBuffer != 0);

		unsigned nOffset = (nEntry * FAT_DIR_ENTRY_SIZE) % FAT_SECTOR_SIZE;
		TFATDirectoryEntry *pFATEntry = (TFATDirectoryEntry *) &m_pBuffer->Data[nOffset];
		assert (pFATEntry != 0);

		if (   pFATEntry->Name[0] == FAT_DIR_NAME0_LAST
		    || pFATEntry->Name[0] == FAT_DIR_NAME0_FREE)
		{
			memset (pFATEntry, 0, FAT_DIR_ENTRY_SIZE);
			memcpy (pFATEntry->Name, FATName, FAT_DIR_NAME_LENGTH);

			return pFATEntry;
		}

		m_pCache->FreeSector (m_pBuffer, 1);
		m_pBuffer = 0;

		nEntry++;

		if (   FATType == FAT32
		    && nEntry % nEntriesPerCluster == 0)
		{
			nPrevCluster = nCluster;

			assert (m_pFAT != 0);
			nCluster = m_pFAT->GetClusterEntry (nCluster);
		}
	}

	m_Lock.Release ();
	
	return 0;
}

void CFATDirectory::FreeEntry (boolean bChanged)
{
	assert (m_pBuffer != 0);
	assert (m_pCache != 0);

	if (bChanged)
	{
		m_pCache->MarkDirty (m_pBuffer);
	}

	m_pCache->FreeSector (m_pBuffer, 1);
	m_pBuffer = 0;

	m_Lock.Release ();
}

boolean CFATDirectory::FindFirst (TDirentry *pEntry, TFindCurrentEntry *pCurrentEntry)
{
	assert (pCurrentEntry != 0);
	pCurrentEntry->nEntry = 0;

	return FindNext (pEntry, pCurrentEntry);
}

boolean CFATDirectory::FindNext (TDirentry *pEntry, TFindCurrentEntry *pCurrentEntry)
{
	char szLongFileNameTemp[MAX_LONG_NAME];
	char szLongFileName[MAX_LONG_NAME];
	
	assert (pEntry != 0);
	assert (pCurrentEntry != 0);
	
	szLongFileName[0] = 0;
	szLongFileNameTemp[0] = 0;

	if (pCurrentEntry->nEntry == 0xFFFFFFFF)
	{
		return FALSE;
	}
	
	assert (m_pFATInfo != 0);
	TFATType FATType = m_pFATInfo->GetFATType ();

	unsigned nEntriesPerCluster = 0;
	if (FATType == FAT32)
	{
		if (pCurrentEntry->nEntry == 0)
		{
			pCurrentEntry->nCluster = m_pFATInfo->GetRootCluster ();
		}

		nEntriesPerCluster =   m_pFATInfo->GetSectorsPerCluster ()
				     * FAT_DIR_ENTRIES_PER_SECTOR;
	}

	m_Lock.Acquire ();

	while (1)
	{
		if (FATType == FAT16)
		{
			if (pCurrentEntry->nEntry >= m_pFATInfo->GetRootEntries ())
			{
				break;
			}
		}
		else
		{
			assert (FATType == FAT32);
			if (m_pFAT->IsEOC (pCurrentEntry->nCluster))
			{
				break;
			}
		}
		
		unsigned nSector;
		if (FATType == FAT16)
		{
			nSector =   m_pFATInfo->GetFirstRootSector ()
				  + pCurrentEntry->nEntry / FAT_DIR_ENTRIES_PER_SECTOR;
		}
		else
		{
			assert (FATType == FAT32);
			nSector =   m_pFATInfo->GetFirstSector (pCurrentEntry->nCluster)
				  +   (pCurrentEntry->nEntry % nEntriesPerCluster)
				    / FAT_DIR_ENTRIES_PER_SECTOR;
		}

		assert (m_pCache != 0);
		TFATBuffer *pBuffer = m_pCache->GetSector (nSector, 0);
		assert (pBuffer != 0);

		unsigned nOffset = (pCurrentEntry->nEntry * FAT_DIR_ENTRY_SIZE) % FAT_SECTOR_SIZE;
		TFATDirectoryEntry *pFATEntry = (TFATDirectoryEntry *) &pBuffer->Data[nOffset];
		assert (pFATEntry != 0);

		if (pFATEntry->Name[0] == FAT_DIR_NAME0_LAST)
		{
			m_pCache->FreeSector (pBuffer, 1);

			break;
		}

		boolean bFound = FALSE;
		
		if (pFATEntry->nAttributes == FAT_DIR_ATTR_LONG_NAME)
		{
			if((pFATEntry->Name[0] & 0x40))
			{
				memset(szLongFileName, 0, MAX_LONG_NAME);
				memset(szLongFileNameTemp, 0, MAX_LONG_NAME);
			}
			if (pFATEntry->Name[0] > 0)
			{
				if (strlen(szLongFileName)>0)
					strcpy(szLongFileNameTemp, szLongFileName);
				LFN *stLFN = (LFN *)pFATEntry;
				//memcpy(&stLFN, pFATEntry, 32);
				#if 1
				char szNamePart1[6] = {0};
				wstrcpy(szNamePart1, stLFN->fileName_Part1, 5);
//				szNamePart1[0] = (char)stLFN->fileName_Part1[0];
//				szNamePart1[1] = (char)stLFN->fileName_Part1[1];
//				szNamePart1[2] = (char)stLFN->fileName_Part1[2];
//				szNamePart1[3] = (char)stLFN->fileName_Part1[3];
//				szNamePart1[4] = (char)stLFN->fileName_Part1[4];
//				szNamePart1[5] = '\0';
				strcpy(szLongFileName, szNamePart1);
				#else
				for(int i = 0; i < 5; i++)
					szLongFileName[pos++] = 0xff & stLFN->fileName_Part1[i];
				#endif
				FMark marker;
				memcpy(&marker, stLFN->fileName_Part2, 12);
				if((marker._Mark1 == 0xffffffff) && (marker._Mark1 == 0xffffffff) && (marker._Mark1 == 0xffffffff))
				{
				} else
				{
					#if 1
					char szNamePart2[7] = {0};
					wstrcpy(szNamePart2, stLFN->fileName_Part2, 6);
					// szNamePart2[0] = (char)stLFN->fileName_Part2[0];
					// szNamePart2[1] = (char)stLFN->fileName_Part2[1];
					// szNamePart2[2] = (char)stLFN->fileName_Part2[2];
					// szNamePart2[3] = (char)stLFN->fileName_Part2[3];
					// szNamePart2[4] = (char)stLFN->fileName_Part2[4];
					// szNamePart2[5] = (char)stLFN->fileName_Part2[5];
					// szNamePart2[6] ='\0';
					strcat(szLongFileName, szNamePart2);			
					#else
					for(int i = 0; i < 7; i++)
						szLongFileName[pos++] = 0xff & stLFN->fileName_Part2[i];
					#endif
				}
				if((stLFN->fileName_Part3[0] & stLFN->fileName_Part3[1] & stLFN->fileName_Part3[2] & stLFN->fileName_Part3[3]) == 0xff)
				{
				 // This is the marker for the end
				 // of the LFN entry for the file
				 // in the root directory.
				 // Only for check. Don't do anything
				}
				else
				{
					#if 1
					char szNamePart3[3]= {0};
					wstrcpy(szNamePart3, stLFN->fileName_Part3, 2);
					szNamePart3[0] = stLFN->fileName_Part3[0];
					szNamePart3[1] = stLFN->fileName_Part3[2];
					szNamePart3[2] ='\0';
					strcat(szLongFileName, szNamePart3);
					#else
					szLongFileName[pos++] = 0xff & stLFN->fileName_Part3[0];
					szLongFileName[pos++] = 0xff & stLFN->fileName_Part3[1];
					szLongFileName[pos] = 0;
					#endif
				}
				strcat(szLongFileName, szLongFileNameTemp);
				//nMaxLFNEntryForFile--;
			}
		}
		else if ( pFATEntry->Name[0] != FAT_DIR_NAME0_FREE && !(pFATEntry->nAttributes & (FAT_DIR_ATTR_VOLUME_ID | FAT_DIR_ATTR_DIRECTORY)))
		{
			FAT2Name ((const char *) pFATEntry->Name, pEntry->chTitle);
			
			strcpy(pEntry->chLongTitle, szLongFileName);
			
			pEntry->nSize = pFATEntry->nFileSize;

			pEntry->nAttributes = FS_ATTRIB_EXECUTABLE;
			if (pFATEntry->nAttributes & FAT_DIR_ATTR_HIDDEN)
			{
				pEntry->nAttributes |= FS_ATTRIB_SYSTEM;
			}

			bFound = TRUE;
		} else
			szLongFileName[0] = 0;

		m_pCache->FreeSector (pBuffer, 1);

		pCurrentEntry->nEntry++;

		if (   FATType == FAT32
		    && pCurrentEntry->nEntry % nEntriesPerCluster == 0)
		{
			assert (m_pFAT != 0);
			pCurrentEntry->nCluster = m_pFAT->GetClusterEntry (pCurrentEntry->nCluster);
		}

		if (bFound)
		{
			m_Lock.Release ();

			return TRUE;
		}
	}

	pCurrentEntry->nEntry = 0xFFFFFFFF;

	m_Lock.Release ();

	return FALSE;
}

unsigned CFATDirectory::Time2FAT (unsigned nTime)
{
	if (nTime == 0)
	{
		return 0;
	}

	unsigned nSecond = nTime % 60;
	nTime /= 60;
	unsigned nMinute = nTime % 60;
	nTime /= 60;
	unsigned nHour = nTime % 24;
	nTime /= 24;

	unsigned nYear = 1970;
	while (1)
	{
		unsigned nDaysOfYear = CTimer::IsLeapYear (nYear) ? 366 : 365;
		if (nTime < nDaysOfYear)
		{
			break;
		}

		nTime -= nDaysOfYear;
		nYear++;
	}

	if (nYear < 1980)
	{
		return 0;
	}
	
	unsigned nMonth = 0;
	while (1)
	{
		unsigned nDaysOfMonth = CTimer::GetDaysOfMonth (nMonth, nYear);
		if (nTime < nDaysOfMonth)
		{
			break;
		}

		nTime -= nDaysOfMonth;
		nMonth++;
	}

	unsigned nMonthDay = nTime + 1;

	unsigned nFATDate = (nYear-1980) << 9;
	nFATDate |= (nMonth+1) << 5;
	nFATDate |= nMonthDay;

	unsigned nFATTime = nHour << 11;
	nFATTime |= nMinute << 5;
	nFATTime |= nSecond / 2;

	return nFATDate << 16 | nFATTime;
}

// TODO: standard FAT name conversion
boolean CFATDirectory::Name2FAT (const char *pName, char *pFATName)
{
	assert (pName != 0);
	assert (pFATName != 0);

	memset (pFATName, ' ', FAT_DIR_NAME_LENGTH);

	const char *pFrom;
	char *pTo;
	unsigned nLength;
	for (pFrom = pName, pTo = pFATName, nLength = 8; *pFrom != '\0'; pFrom++)
	{
		char c = *pFrom;

		if (c <= ' ')
		{
			return FALSE;
		}

		static const char *pBadChars = "\"*+,/:;<=>?[\\]|";
		for (const char *pBad = pBadChars; *pBad; pBad++)
		{
			if (c == *pBad)
			{
				return FALSE;
			}
		}

		if ('a' <= c && c <= 'z')
		{
			c -= 'a'-'A';
		}

		if (c == '.')
		{
			pTo = pFATName+8;
			nLength = 3;
			continue;
		}

		if (nLength > 0)
		{
			*pTo++ = c;
			nLength--;
		}
	}

	if (pFATName[0] == ' ')
	{
		return FALSE;
	}

	return TRUE;
}

void CFATDirectory::FAT2Name (const char *pFATName, char *pName)
{
	assert (pFATName != 0);
	assert (pName != 0);

	char FATName[FAT_DIR_NAME_LENGTH+1];
	strncpy (FATName, pFATName, FAT_DIR_NAME_LENGTH);
	FATName[FAT_DIR_NAME_LENGTH] = '\0';
	
	if (FATName[0] == FAT_DIR_NAME0_KANJI_E5)
	{
		FATName[0] = '_';
	}

	for (char *p = FATName; *p != '\0'; p++)
	{
		if (*p == ' ')
		{
			*p = '\0';
		}
		else if ('A' <= *p && *p <= 'Z')
		{
			*p += 'a'-'A';
		}
	}

	strncpy (pName, FATName, 8);
	pName[8] = '\0';

	if (FATName[8] != '\0')
	{
		strcat (pName, ".");
		strcat (pName, FATName+8);
	}
}
