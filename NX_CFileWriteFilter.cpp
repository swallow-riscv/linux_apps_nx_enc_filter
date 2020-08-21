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
#endif

#include <stdio.h>
#include <string.h>
#include <fcntl.h> 			// for O_RDWR
#include <sched.h> 			// schedule

#include "NX_SystemCall.h"
#include "NX_CFileWriteFilter.h"

#define		DTAG "[FileWrite]"
#include "NX_DebugMsg.h"

//////////////////////////////////////////////////////////////////////////////
//
//							NX_CFileWriteFilter
//
NX_CFileWriteFilter::NX_CFileWriteFilter() :
	m_pInputPin(NULL),
	m_pInQueue(NULL)
{
	m_pInputPin = (NX_CVideoRenderInputPin*)new NX_CVideoRenderInputPin( this );
	m_pInQueue = new NX_CSampleQueue(MAX_NUM_IN_QUEUE);
	NX_DbgMsg(DBG_INFO, "NX_CFileWriteFilter()\n");
}

NX_CFileWriteFilter::~NX_CFileWriteFilter()
{
	if( m_pInputPin )
		delete m_pInputPin;
	if( m_pInQueue )
		delete m_pInQueue;
	NX_DbgMsg(DBG_INFO, "~NX_CFileWriteFilter()\n");
}


NX_CBasePin *NX_CFileWriteFilter::GetPin( int Pos )
{
	if( 0 == Pos && NULL != m_pInputPin )
	{
		return m_pInputPin;
	}
	return NULL;
}

int32_t	NX_CFileWriteFilter::RenderSample( NX_CSample *pSample )
{
	//	Push Sample
	if( m_pInQueue )
	{
		pSample->AddRef();
		return m_pInQueue->PushSample( pSample );
	}
	return -1;
}

bool NX_CFileWriteFilter::Run()
{
	if( false == m_bRunning )
	{
		m_pInQueue->ResetQueue();
		if( m_pInputPin )
			m_pInputPin->Active();

		m_bPaused = false;
		m_bExitThread = false;

		if( pthread_create( &m_hThread, NULL, ThreadStub, this ) < 0 )
		{
			return false;
		}
		m_bRunning = true;
	}
	return true;
}


bool NX_CFileWriteFilter::Stop()
{
	NX_DbgMsg( DBG_INFO, " Stop() ++\n" );

	//	Set Thread End Command
	ExitThread();
	//	Send Queue Emd Message
	if( m_pInQueue )
		m_pInQueue->EndQueue();

	//	Input Inactive
	if( m_pInputPin )
		m_pInputPin->Inactive();

	if( true == m_bRunning )
	{
		NX_DbgMsg(DBG_TRACE, " Stop() : pthread_join() ++\n" );
		m_bExitThread = true;
		pthread_join( m_hThread, NULL );
		m_bRunning = false;
		NX_DbgMsg( DBG_TRACE, " Stop() : pthread_join() --\n" );
	}
	NX_DbgMsg( DBG_INFO, " Stop() --\n" );
	return true;
}

void NX_CFileWriteFilter::Pause(void)
{
	m_bPaused = true;
}
void NX_CFileWriteFilter::Resume(void)
{
	m_bPaused = false;
	m_SemPause.Post();
}


void *NX_CFileWriteFilter::ThreadStub( void *pObj )
{
	if( NULL != pObj )
	{
		((NX_CFileWriteFilter*)pObj)->ThreadProc();
	}
	return (void*)(0);
}


void NX_CFileWriteFilter::ThreadProc()
{
	NX_CSample *pSample;
	int32_t bInit = false;
	FILE *hFile = NULL;
	uint8_t *pBuf;
	int32_t bufSize;

	uint64_t frameCnt = 0;
	int64_t startTime, deltaTime;

	if( !m_pInQueue )
		return ;

	hFile = fopen( m_FileName, "wb");

	while(1)
	{
		while( m_bPaused && !m_bExitThread )
		{
			NX_DbgMsg( DBG_INFO, "Paused.\n" );
			m_SemPause.Pend( 500000 );
		}
		if( m_bExitThread ){ break; }

		pSample = NULL;
		if( m_pInQueue->PopSample( &pSample ) <  0)		break;

		if( !pSample )									break;


		pSample->GetPointer((void**)&pBuf, &bufSize );

		if( hFile )
		{
			if( frameCnt == 0 )
			{
				startTime = NX_GetTickCount();
			}
			if( pSample->IsSyncPoint() )
//			if( frameCnt % 30 == 29  )
			{
				float fps = 0;
				deltaTime = NX_GetTickCount() - startTime;

				if( deltaTime > 0 )
				{
					fps = (frameCnt * 1000) / deltaTime;
				}
				NX_DbgMsg( DBG_DEBUG ,DTAG"Receive Key Frame(avg fps = %f) \n", fps);
			}

			//fwrite( pBuf, 1, pSample->GetActualDataLength(), hFile );

			frameCnt ++;
		}

		pSample->Release();
	}

	NX_DbgMsg( DBG_INFO, "Exit renderer thread!!\n" );

}


bool NX_CFileWriteFilter::SetFileName( const char *pBuf )
{
	if( NULL == pBuf || (MAX_PATH-1) < strlen( pBuf ) )
		return false;
#ifdef _WIN32
	strncpy_s( m_FileName, sizeof(m_FileName), pBuf );
#else
	strcpy( m_FileName, pBuf );
#endif

	NX_DbgMsg( DBG_INFO, "SetFileName : %s\n", m_FileName );

	return true;
}


//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//
//						Nexell CRender Input Pin Class
//
NX_CVideoRenderInputPin::NX_CVideoRenderInputPin(	NX_CFileWriteFilter *pOwnerFilter ) :
	NX_CBaseInputPin( (NX_CBaseFilter*)pOwnerFilter )
{
}
NX_CVideoRenderInputPin::~NX_CVideoRenderInputPin()
{
}
int32_t NX_CVideoRenderInputPin::Receive( NX_CSample *pSample )
{
	if( NULL == m_pOwnerFilter || true != IsActive() )
		return -1;
	return ((NX_CFileWriteFilter*)m_pOwnerFilter)->RenderSample( pSample );
}
//
//////////////////////////////////////////////////////////////////////////////

