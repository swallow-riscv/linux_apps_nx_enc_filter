
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

#include <stdint.h>

#include "NX_EncoderManager.h"

#define DTAG	"[ENC_MGR]"
#include "NX_DebugMsg.h"

#define MAJOR_VERSION		1
#define MINOR_VeRSION		0
#define SVN_REVISION		"$Rev: 1816 $"
#define SVN_DATE			"$Date: 2011-10-31 22:39:13 +0900 (월, 31 10 2011) $"

NX_EncoderMgr::NX_EncoderMgr( int32_t Width, int32_t Height, int32_t numFiles )
	//	Initialize Filters
	: m_pCapture(NULL)
	, m_pEncoder(NULL)
	, m_pWriter(NULL)

	, m_pRefClock(NULL)
	//	Initalize Pins
	, m_pPinVipOut(NULL)
	, m_pPinEncIn(NULL)
	, m_pPinEncOut(NULL)
	, m_pPinWriterIn(NULL)

	, m_bRunning(false)

	, EventCallback(NULL)
	, m_bEventThreadExit(true)
	, m_IsEventThread(true)
	, m_iHeadIndex(0)
	, m_iTailIndex(0)
	, m_MsgCount(0)
	, m_QueueDepth(128)
	, m_NumFiles(numFiles)

{
	int32_t ErrorCode = 0;

	//	Set Default Video Resolution
	m_Width = Width;
	m_Height= Height;


	//	create event handler
	m_bEventThreadExit = false;
	//m_pEventSem = new NX_CSemaphore( m_QueueDepth, 0 );
	pthread_mutex_init( &m_hEventLock, 0 );
	if( 0 > pthread_create( &m_hEventThread, NULL, EventProcStub, this ) )
	{
		NX_ErrMsg( "%s:%d : Cannot create event process thread!!\n", __FILE__, __LINE__ );
		//if( m_pEventSem )	delete m_pEventSem;
		//m_pEventSem = NULL;
	}

	m_pCapture = new NX_CVipFilter;
	if( m_pCapture )
	{
		m_pPinVipOut = m_pCapture->GetPin(0);
	}

	m_pEncoder = new NX_CVideoEncoderFilter(m_Width, m_Height, &ErrorCode);
	{
		m_pPinEncIn  = m_pEncoder->GetPin(0);
		m_pPinEncOut = m_pEncoder->GetPin(1);
	}

	m_pWriter = new NX_CFileWriteFilter;
	if( m_pWriter )
	{
		m_pPinWriterIn = m_pWriter->GetPin(0);
	}

	m_pRefClock		= new NX_CClockRef;

	ConnectPin( m_pPinVipOut, m_pPinEncIn);
	ConnectPin( m_pPinEncOut, m_pPinWriterIn);

	NX_SetClockReference();
	NX_SetEventNotifier();
}

void NX_EncoderMgr::NX_SetEventNotifier()
{
	m_EventNotifier.SetEventReceiver( this );

	if( NULL != m_pCapture )	m_pCapture->SetEventNotifier( &m_EventNotifier );
	if( NULL != m_pEncoder )	m_pEncoder->SetEventNotifier( &m_EventNotifier );
	if( NULL != m_pWriter  )	m_pWriter ->SetEventNotifier( &m_EventNotifier );
}

void NX_EncoderMgr::NX_SetClockReference()
{
	if( NULL != m_pRefClock  && NULL != m_pCapture )	m_pCapture->SetClockRef( m_pRefClock );
	if( NULL != m_pRefClock  && NULL != m_pEncoder )	m_pEncoder->SetClockRef( m_pRefClock );
	if( NULL != m_pRefClock  && NULL != m_pWriter  )	m_pWriter ->SetClockRef( m_pRefClock );
}


NX_EncoderMgr::~NX_EncoderMgr()
{
	if( m_bRunning )
		Stop();

	if( m_pCapture )		delete m_pCapture;
	if( m_pEncoder )		delete m_pEncoder;
	if( m_pWriter  )		delete m_pWriter ;

	if( m_pRefClock )	delete m_pRefClock;

	m_bEventThreadExit = true;
	if( m_pEventSem ){
		pthread_join( m_hEventThread, NULL );
	}
	pthread_mutex_destroy( &m_hEventLock );

}

int32_t NX_EncoderMgr::Run(void)
{
	NX_CAutoLock Lock(m_ControlLock);
	NX_DbgMsg( DBG_TRACE, DTAG"Run() ++\n");

	if( !m_bRunning ){

		if( m_pCapture )	m_pCapture->Run();
		if( m_pEncoder )	m_pEncoder->Run();
		if( m_pWriter  )	m_pWriter ->Run();

		m_bRunning = true;
	}
	NX_DbgMsg( DBG_TRACE, DTAG"Run() --\n");

	return 0;
}

int32_t NX_EncoderMgr::Stop(void)
{
	NX_CAutoLock Lock(m_ControlLock);
	NX_DbgMsg( DBG_TRACE, "Stop() ++\n" );
	if( m_bRunning ){
		if( m_pCapture  )	m_pCapture->Stop();
		if( m_pEncoder  )	m_pEncoder->Stop();
		if( m_pWriter   )	m_pWriter ->Stop();
		if( m_pRefClock )	m_pRefClock->Reset();
	}
	NX_DbgMsg( DBG_TRACE, "Stop() --\n" );
	m_bRunning = false;
	return 0;
}

int32_t NX_EncoderMgr::Pause(void)
{
	NX_CAutoLock Lock(m_ControlLock);
	NX_DbgMsg( DBG_TRACE, "Pause() ++\n" );
	NX_DbgMsg( DBG_TRACE, "Pause() --\n" );
	return 0;
}

int32_t NX_EncoderMgr::Resume(void)
{
	NX_CAutoLock Lock(m_ControlLock);
	NX_DbgMsg( DBG_TRACE, "Resume() ++\n" );
	NX_DbgMsg( DBG_TRACE, "Resume() --\n" );
	return 0;
}

int32_t NX_EncoderMgr::Seek( int64_t time )
{
	NX_CAutoLock Lock(m_ControlLock);
	NX_DbgMsg( DBG_TRACE, "Seek() ++\n" );
	NX_DbgMsg( DBG_TRACE, "Seek() --\n" );
	return 0;
}

int32_t NX_EncoderMgr::CurrentTime( int64_t *retTime )
{
	NX_CAutoLock Lock(m_ControlLock);
	if( m_bRunning ){
		if( m_pRefClock ){
			int64_t curTime = 0;
			if( m_pRefClock->GetMediaTime(&curTime) == 0 ){
				*retTime = curTime;
				return 0;
			}
		}
	}
	return -1;
}

int NX_EncoderMgr::PushEvent( uint32_t EventCode, uint32_t Value )
{
	int ret = 0;
	pthread_mutex_lock( &m_hEventLock );
	if( m_bEventThreadExit ){
		ret = -1;
		goto exit_unlock;
	}
	m_EventMsg[m_iTailIndex].eventType = EventCode;
	m_EventMsg[m_iTailIndex].eventData = Value;
	m_iTailIndex = (m_iTailIndex+1) % m_QueueDepth;
	m_MsgCount++;

exit_unlock:
	pthread_mutex_unlock( &m_hEventLock );
	return ret;
}

int NX_EncoderMgr::PopEvent( uint32_t &EventCode, uint32_t &Value  )
{
	int ret = 0;
	pthread_mutex_lock( &m_hEventLock );

	if( m_bEventThreadExit ){
		ret = -1;
		goto exit_unlock;
	}

	EventCode = m_EventMsg[m_iHeadIndex].eventType;
	Value = m_EventMsg[m_iHeadIndex].eventData;
	m_EventMsg[m_iHeadIndex].eventType = 0;
	m_EventMsg[m_iHeadIndex].eventData = 0;
	m_iHeadIndex = (m_iHeadIndex+1) % m_QueueDepth;
	m_MsgCount--;

exit_unlock:
	pthread_mutex_unlock( &m_hEventLock );
	return ret;
}

void NX_EncoderMgr::ProcessEvent( uint32_t EventCode, uint32_t Value )
{
//	if( m_pEventSem ){
		PushEvent( EventCode, Value );
		//m_pEventSem->Post();
//	}else{
//		NX_ErrMsg(("Error : cannot process event.(m_pEventSemp=%p, Code=0x%08x, Value=0x%08x)\n", m_pEventSem, EventCode, Value ));
//	}
}

void *NX_EncoderMgr::EventProcStub( void *param )
{
	NX_EncoderMgr *pMgr = (NX_EncoderMgr *)param;
	if( pMgr ){
		return pMgr->EventProc();
	}
	return (void*)NULL;
}

void *NX_EncoderMgr::EventProc()
{
	uint32_t EventCode, Value;
	m_IsEventThread = true;
	while( !m_bEventThreadExit )
	{
		//if( m_pEventSem->Pend(1000000) < 0 )
		//	break;

		if( m_MsgCount<=0 )
		{
			NX_Sleep( 100000 );
			continue;
		}

		if( m_bEventThreadExit )
			break;

		//	processing
		if( 0 == PopEvent( EventCode, Value ) )
		{
			if( m_bEventThreadExit )
				break;
			switch( EventCode )
			{
				case EVENT_ENABLE_VIDEO:
					//if( Value == EVENT_ACTION_EANBLE ){
					//	if( m_pVRend ) m_pVRend->EnableVideoLayer(1);
					//}else{
					//	if( m_pVRend ) m_pVRend->EnableVideoLayer(0);
					//}
					if( EventCallback )
						EventCallback( m_cbObj, EVENT_ENABLE_VIDEO, 0 );
					break;
				case EVENT_CHANGE_STREAM_INFO:
					break;
				case EVENT_END_OF_STREAM:
					NX_DbgMsg( DBG_TRACE, "[EventHandler] End of stream(%d)\n", Value );
					break;
				default:
					NX_DbgMsg( DBG_TRACE, "[EventHandler] Unknwon defined event\n" );
					break;
			}
		}
	}

	return (void*)0xDEADDEAD;
}
