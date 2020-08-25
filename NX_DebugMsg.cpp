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
#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "NX_DebugMsg.h"

static unsigned int gst_TargetDebugLevel = PLAYER_DBG_LEVEL_INFO;	//	Error
void NX_ChangeDebugLevel( unsigned int TargetLevel )
{
	printf(DTAG ">>> NX_ChangeDebugLevel : %d to %d\n", gst_TargetDebugLevel, TargetLevel);
	gst_TargetDebugLevel = TargetLevel;
}

void NX_DbgMsg( unsigned int level, const char *format, ... )
{
	char buffer[1024];
	if( level > gst_TargetDebugLevel )	return;

	{
		va_list marker;
		va_start(marker, format);
#ifdef _WIN32
		vsprintf_s(buffer, format, marker);
#else
		vsprintf(buffer, format, marker);
#endif
		printf( " %s", buffer );
#ifdef _WIN32
		OutputDebugStringA(buffer);
#endif
		va_end(marker);
	}
}


void NX_RelMsg( const char *format, ... )
{
	char buffer[1024];
	va_list marker;
	va_start(marker, format);
#ifdef _WIN32
	vsprintf_s(buffer, format, marker);
#else
	vsprintf(buffer, format, marker);
#endif
	printf(" %s", buffer );
#ifdef _WIN32
	OutputDebugStringA(buffer);
#endif
	va_end(marker);
}


void NX_ErrMsg(const char* format, ...)
{
	char buffer[1024];
	va_list marker;
	va_start(marker, format);
#ifdef _WIN32
	vsprintf_s(buffer, format, marker);
#else
	vsprintf(buffer, format, marker);
#endif
	printf("ERROR : %s", buffer);
#ifdef _WIN32
	OutputDebugStringA(buffer);
#endif
	va_end(marker);
}
