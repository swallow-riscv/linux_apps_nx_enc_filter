#ifndef __NX_EncoderManager_h__
#define __NX_EncoderManager_h__
#include "NX_PlayerConfig.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdio.h>
#include <stdint.h>

#include "NX_SystemCall.h"
#include "NX_CVipFilter.h"
#include "NX_CFileWriteFilter.h"
#include "NX_CVideoEncoderFilter.h"


typedef struct tag_PLAYER_EVENT_MESSAGE
{
	uint32_t		eventType;
	uint32_t		eventData;
} PLAYER_EVENT_MESSAGE;


class NX_EncoderMgr : public NX_CEventReceiver
{
public:
	NX_EncoderMgr(int32_t Width, int32_t Height, int32_t initQP, int32_t numFiles);
	~NX_EncoderMgr();

public:
	int32_t Run(void);
	int32_t Stop(void);
	int32_t Pause(void);
	int32_t Resume(void);
	int32_t Seek( int64_t time );
	int32_t CurrentTime( int64_t *time );

	int32_t SetFileName( const char *FileName, int32_t frames, int32_t outfiles ){
		if( m_pWriter )	return m_pWriter->SetFileName( FileName, frames, outfiles );
		return -1;
	}

	int32_t SetCameraInfo( int32_t deviceId, int32_t width, int32_t height )
	{
		if( m_pCapture )
			m_pCapture->SetCameraInfo( deviceId, width, height );
		return 0;
	}

	int32_t	SetEncoderInfo( int32_t width, int32_t height, int32_t fps, int32_t gop, uint32_t bitrate, int32_t initQP )
	{
		if( m_pEncoder )
			return m_pEncoder->SetEncoderInfo( width, height, fps, gop, bitrate, initQP );
		return -1;
	}

	int32_t SetEventCallback( void *obj, void (*notifyFunc)(void*,uint32_t,uint32_t) )
	{
		if( notifyFunc ){
			m_cbObj = obj;
			EventCallback = notifyFunc;
		}
		return -1;
	}

	static void *EventProcStub( void *param );
	void *EventProc();

private:
	NX_CVipFilter				*m_pCapture;
	NX_CVideoEncoderFilter		*m_pEncoder;
	NX_CFileWriteFilter			*m_pWriter;
	NX_CClockRef				*m_pRefClock;

	NX_CBasePin					*m_pPinVipOut;
	NX_CBasePin					*m_pPinEncIn, *m_pPinEncOut;
	NX_CBasePin					*m_pPinWriterIn;

	NX_CEventNotifier			m_EventNotifier;

	int32_t						m_Width;
	int32_t						m_Height;
	int32_t						m_Fps;
	int32_t						m_Gop;
	uint32_t					m_Bitrate;
	int32_t						m_InitQP;
	bool						m_bRunning;


	//	Event Callback Function Pointer
	void (*EventCallback)( void *Obj, uint32_t EventType, uint32_t EventData );

	int PushEvent( uint32_t EventCode, uint32_t Value );
	int PopEvent( uint32_t &EventCode, uint32_t &Value  );

	void						*m_cbObj;
	bool						m_bEventThreadExit;
	bool						m_IsEventThread;
	pthread_t					m_hEventThread;
	NX_CSemaphore				*m_pEventSem;
	PLAYER_EVENT_MESSAGE		m_EventMsg[128];
	int							m_iHeadIndex, m_iTailIndex, m_MsgCount, m_QueueDepth;
	pthread_mutex_t				m_hEventLock;

	bool ConnectPin( NX_CBasePin *pOutPin, NX_CBasePin *pInPin )
	{
		if( NULL != pOutPin && NULL != pInPin ){
			pOutPin->Connect( pInPin );
			pInPin->Connect( pOutPin );
			return true;
		}
		return false;
	}

	virtual void ProcessEvent( uint32_t EventCode, uint32_t Value );
	void NX_SetEventNotifier();
	void NX_SetClockReference();


private:
	NX_CMutex					m_ControlLock;

	int32_t						m_FolderMode;
	int32_t						m_NumFiles;

	NX_EncoderMgr (NX_EncoderMgr& Ref);
	NX_EncoderMgr &operator=(NX_EncoderMgr &Ref);
};


#endif	//	__NX_EncoderManager_h__
