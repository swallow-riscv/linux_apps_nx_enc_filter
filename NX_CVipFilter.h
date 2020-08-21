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
#ifndef __NX_CVipFilter_h__
#define __NX_CVipFilter_h__

#ifdef __cplusplus

#include "NX_CBaseFilter.h"

#define		MAX_VIDEO_NUM_BUF	(8)
#define		MAX_AUDIO_NUM_BUF	(4)

#define		MAX_VIDEO_SIZE_BUF	(4*1024)		//	4K : Structure Size for Video Memory
//#define		MAX_VIDEO_SIZE_BUF	(1024*1024*3)		//	4MB
#define		MAX_AUDIO_SIZE_BUF	(1024*512)			//	512K

#include <stdint.h>
#include <pthread.h>

#include <linux/videodev2.h>
#include <linux/media-bus-format.h>
#include <nx-v4l2.h>
#include <nx_video_alloc.h>

class NX_CSourceOutputPin;

//////////////////////////////////////////////////////////////////////////////
//
//							Nexell VIP Filter
class NX_CVipFilter : 
	public NX_CBaseFilter
{
public:
	NX_CVipFilter();
	virtual ~NX_CVipFilter();

public:
	//	overide NX_CBaseFilter
	virtual bool Run();
	virtual bool Stop();
	virtual NX_CBasePin	*GetPin( int Pos );

	//	Thread
	pthread_t			m_hThread;
	bool				m_bExitThread;
	pthread_mutex_t		m_hMutexThread;

	static void *ThreadStub( void *pObj );
	bool StartThreadProc();
	void EndThreadProc();
	void ThreadProc();
	void ExitThread(){
		pthread_mutex_lock(&m_hMutexThread);
		m_bExitThread = true;
		pthread_mutex_unlock(&m_hMutexThread);
	}

	void DeliverEndOfStream( void );

	void SetCameraInfo( int32_t devId, int32_t width, int32_t height )
	{
		m_DeviceId = devId;
		m_Width    = width;
		m_Height   = height;
	}

	virtual int32_t ReleaseSample( NX_CSample *pSample );

	NX_CSampleQueue		*m_pReleaseQueue;

	NX_CSourceOutputPin	*m_pVideoOut;
	int32_t				m_NumOutputPin;

	int32_t				m_DeviceId;
	int32_t				m_Width;
	int32_t				m_Height;
	enum	{ MAX_CAMERA_BUFFER = 6 };
	NX_VID_MEMORY_HANDLE m_hCamMem[MAX_CAMERA_BUFFER];

private:
	NX_CVipFilter (NX_CVipFilter &Ref);
	NX_CVipFilter &operator=(NX_CVipFilter &Ref);
};
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//						Parser Output Pin Class
//
typedef enum{	OUT_PIN_TYPE_VIDEO, OUT_PIN_TYPE_AUDIO } NX_OUT_PIN_TYPE;
class NX_CSourceOutputPin:
	public NX_CBaseOutputPin
{
public:
	NX_CSourceOutputPin( NX_CBaseFilter *pOwnerFilter, NX_OUT_PIN_TYPE PinType );
	virtual ~NX_CSourceOutputPin();

	//	Implementation Pure Virtual Functions
public:
	virtual int32_t		ReleaseSample( NX_CSample *pSample );
	virtual int32_t		GetDeliveryBuffer(NX_CSample **pSample);

public:
	//	Stream Pumping Thread
	pthread_t		m_hThread;
	bool			m_bExitThread;
	pthread_mutex_t	m_hMutexThread;
	static void		*ThreadStub( void *pObj );
	void ThreadProc();
	void ExitThread(){
		pthread_mutex_lock(&m_hMutexThread);
		m_bExitThread = true;
		pthread_mutex_unlock(&m_hMutexThread);
	}

	//	Override
	bool Active();
	bool Inactive();

	//	Alloc Buffer
	int32_t	AllocBuffers();
	int32_t	DeallocBuffers();

	NX_OUT_PIN_TYPE	m_PinType;

	//	Media Sample Queue
	NX_CSampleQueue	*m_pInQueue;		//	Input Free Queue
	NX_CSampleQueue	*m_pOutQueue;		//	Output Queue
	NX_CSample		*m_pSampleList;

private:
	NX_CSourceOutputPin (NX_CSourceOutputPin &Ref);
	NX_CSourceOutputPin &operator=(NX_CSourceOutputPin &Ref);
};

//
//////////////////////////////////////////////////////////////////////////////



#endif	//	__cplusplus

#endif	//	__NX_CVipFilter_h__
