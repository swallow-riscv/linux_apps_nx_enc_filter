#include "pch.h"

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
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#else
#endif

#include "NX_SystemCall.h"

#ifdef _WIN32

void NX_Sleep( unsigned int MilliSeconds )
{
	Sleep( MilliSeconds );
}
long long NX_GetTickCount()
{
	return ((long long)GetTickCount());
}

#endif