//------------------------------------------------------------------------------
//
//	Copyright (C) 2010 Nexell co., Ltd All Rights Reserved
//	Nexell Proprietary & Confidential
//
//	Module     : 
//	File       : 
//	Description:
//	Author     : RayPark
//	History    :
//------------------------------------------------------------------------------
#include "NX_SystemCall.h"

#ifdef __linux__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>

//#define	USE_USLEEP


//////////////////////////////////////////////////////////////////////////////
//
//							Sleep & Time
//
void NX_Sleep( unsigned int MilliSeconds )
{
#ifdef USE_USLEEP 
	usleep( MilliSeconds * 1000 );
#else
	struct timeval	tv;
	tv.tv_sec = 0;
	tv.tv_usec = MilliSeconds * 1000;
	select( 0, NULL, NULL, NULL, &tv );
#endif
}

int64_t NX_GetTickCount()
{
	int64_t Ret;
	struct timeval	tv;
	struct timezone	zv;
	gettimeofday( &tv, &zv );
	Ret = ((int64_t)tv.tv_sec)*1000 + (int64_t)(tv.tv_usec/1000);
	return Ret;
}



void hexdump(const void *_data, int32_t size)
{
	const uint8_t *data = (const uint8_t *)_data;
	int32_t offset = 0;
	char tmp[64];
	char line[128];
	int i;
	while (offset < size)
	{
		line[0] = 0;
		snprintf(tmp, sizeof(tmp), "%08x:  ", (uint32_t)offset);
		strcat( line, tmp );

		for (i = 0; i < 16; ++i)
		{
			if (i == 8) {
				strcat( line, tmp );
			}
			if (offset + i >= size) {
				strcat( line, "   ");
			} else {
				snprintf(tmp, sizeof(tmp), "%02x ", data[offset + i]);
				strcat( line, tmp );
			}
		}

		strcat(line, " ");
		for (i = 0; i < 16; ++i) {
			if (offset + i >= size) {
				break;
			}
			if (isprint(data[offset + i])) {
				strncat(line, (char*)&data[offset + i], 1);
			} else {
				strcat(line, ".");
			}
		}
		printf("%s\n", line);
		offset += 16;
	}
}

//
//
//////////////////////////////////////////////////////////////////////////////

#endif