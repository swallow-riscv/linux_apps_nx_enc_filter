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
#ifndef __NX_SystemCall_h__
#define __NX_SystemCall_h__

#include <stdint.h>

#include "NX_PlayerConfig.h"

void NX_Sleep( unsigned int MilliSeconds );
int64_t NX_GetTickCount();
void hexdump(const void *_data, int32_t size);

#endif	//	__NX_SystemCall_h__
