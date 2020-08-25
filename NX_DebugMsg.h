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
#ifndef __NX_DebugMsg_h__
#define __NX_DebugMsg_h__

#ifndef DTAG
#define	DTAG	"[CAM_ENCODER]"
#endif

#define	PLAYER_DBG_LEVEL_DISABLE	0
#define PLAYER_DBG_LEVEL_ERROR		1
#define PLAYER_DBG_LEVEL_WARNING	2
#define PLAYER_DBG_LEVEL_INFO		3
#define PLAYER_DBG_LEVEL_DEBUG		4
#define PLAYER_DBG_LEVEL_TRACE		5
#define PLAYER_DBG_LEVEL_VERBOSE	6

void NX_ChangeDebugLevel( unsigned int TargetLevel );
void NX_DbgMsg( unsigned int level, const char *format, ... );
void NX_RelMsg( const char *format, ... );
void NX_ErrMsg(const char* format, ...);


//////////////////////////////////////////////////////////////////////////////
//
//					Player Internal Debug Message
//

#define DBG_ERR			PLAYER_DBG_LEVEL_ERROR
#define DBG_WARN		PLAYER_DBG_LEVEL_WARNING
#define DBG_INFO		PLAYER_DBG_LEVEL_INFO
#define DBG_DEBUG		PLAYER_DBG_LEVEL_DEBUG
#define DBG_TRACE		PLAYER_DBG_LEVEL_TRACE
#define	DBG_VERBOSE		PLAYER_DBG_LEVEL_VERBOSE

#define DBG_MEMORY		DBG_TRACE

#endif	//	__NX_DebugMsg_h__
