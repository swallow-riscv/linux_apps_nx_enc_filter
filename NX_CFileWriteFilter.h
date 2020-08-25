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
#ifndef __NX_CFileWriteFilter_h__
#define __NX_CFileWriteFilter_h__
#include <stdint.h>

#ifdef __cplusplus

#pragma once

#include "NX_CBaseFilter.h"

class NX_CEventNotifier;
class NX_CVideoRenderInputPin;
#ifndef MAX_PATH
#define MAX_PATH	(1024)
#endif

//////////////////////////////////////////////////////////////////////////////
//
//					Nexell Video Renderer Filter
//
class NX_CFileWriteFilter : 
	public NX_CBaseFilter
{
public:
	NX_CFileWriteFilter();
	virtual ~NX_CFileWriteFilter();

public:	//	for Video Rendering
	pthread_t			m_hThread;
	bool				m_bExitThread;
	bool				m_bPaused;
	static void			*ThreadStub( void *pObj );
	void				ThreadProc();
	void				ExitThread(){m_bExitThread = true;}
	int32_t				ReleaseSample( NX_CSample *pSample ){return 0;}

	virtual bool		Run();
	virtual bool		Stop();
	void				Pause(void);
	void				Resume(void);


	bool SetFileName( const char *pBuf, int32_t numFrames, int32_t numOutFiles );


	enum	{ MAX_NUM_IN_QUEUE = 64 };

public:
	NX_CBasePin			*GetPin( int Pos );
	virtual int32_t			RenderSample( NX_CSample *pSample );

//protected:
	NX_CVideoRenderInputPin		*m_pInputPin;
	NX_CSampleQueue				*m_pInQueue;
	NX_CSemaphore				m_SemPause;

	char						m_FileName[MAX_PATH];
	int32_t						m_NumFrames;
	int32_t						m_NumOutFiles;			//	Output File Rotate

	
private:
	NX_CFileWriteFilter (NX_CFileWriteFilter &Ref);
	NX_CFileWriteFilter &operator=(NX_CFileWriteFilter &Ref);
};
//
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//				Nexell Video Renderer Input Pin for Linux
//

class NX_CVideoRenderInputPin: 
	public NX_CBaseInputPin
{
public:
	NX_CVideoRenderInputPin( NX_CFileWriteFilter *pOwnerFilter );
	virtual ~NX_CVideoRenderInputPin();

protected:
	virtual int32_t Receive( NX_CSample *pSample );
};

//
//
//////////////////////////////////////////////////////////////////////////////


#endif	//	__cplusplus

#endif	//	__NX_CFileWriteFilter_h__
