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
#ifndef	__NX_CBaseFitler_h__
#define	__NX_CBaseFitler_h__

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux__
#include <time.h>
#include <assert.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include "NX_EventMessage.h"
#include "NX_SystemCall.h"
#include "NX_DebugMsg.h"


#ifdef __cplusplus

#pragma once

class NX_CSample;
class NX_CBasePin;
class NX_CClockRef;

//----------------------------------------------------------------------------
//		Semaphore
//----------------------------------------------------------------------------
class NX_CSemaphore{
public:
	NX_CSemaphore() :
		m_Value (1),
		m_Max (1),
		m_Init (0),
		m_bReset (false)
	{
		pthread_cond_init ( &m_hCond,  NULL );
		pthread_mutex_init( &m_hMutex, NULL );
	}
	NX_CSemaphore( int32_t Max, int32_t Init ) :
		m_Value (Init),
		m_Max (Max),
		m_Init (Init),
		m_bReset (false)
	{
		pthread_cond_init ( &m_hCond,  NULL );
		pthread_mutex_init( &m_hMutex, NULL );
	}
	~NX_CSemaphore()
	{
		ResetSignal();
		pthread_cond_destroy( &m_hCond );
		pthread_mutex_destroy( &m_hMutex );
	}

	enum {	MAX_SEM_VALUE = 1024	};
public:
	int32_t Post()
	{
		int Ret = 0;
		pthread_mutex_lock( &m_hMutex );
		m_Value ++;
		pthread_cond_signal ( &m_hCond );
		if(m_Value<=0 ){
			Ret = -1;
		}
		pthread_mutex_unlock( &m_hMutex );
		return Ret;
	}
	int32_t Pend(int32_t nSec)
	{
		int32_t Ret = 0;
		pthread_mutex_lock(&m_hMutex);
		if (m_Value == 0) {
			pthread_cond_wait(&m_hCond, &m_hMutex);
			m_Value--;
		}
		else if (m_Value < 0) {
			Ret = -1;
		}
		else {
			m_Value--;
			Ret = 0;
		}
		pthread_mutex_unlock(&m_hMutex);
		return Ret;
	}
	void ResetSignal()
	{
		pthread_mutex_lock ( &m_hMutex );
		m_bReset = true;
		for( int i=0 ; i<m_Max ; i++ ){
			pthread_cond_broadcast( &m_hCond );
		}
		pthread_mutex_unlock( &m_hMutex );
	}
	void ResetValue()
	{
		pthread_mutex_lock( &m_hMutex );
		m_Value = m_Init;
		m_bReset = false;
		pthread_mutex_unlock( &m_hMutex );
	}

private:
	pthread_cond_t  m_hCond;
	pthread_mutex_t m_hMutex;
	int32_t				m_Value;
	int32_t				m_Max;
	int32_t				m_Init;
	bool			m_bReset;
};


class NX_CMutex{
public:
	NX_CMutex()
	{
		pthread_mutex_init( &m_hMutex, NULL );
	}
	~NX_CMutex()
	{
		pthread_mutex_destroy( &m_hMutex );
	}
	void Lock()
	{
		pthread_mutex_lock( &m_hMutex );
	}
	void Unlock()
	{
		pthread_mutex_unlock( &m_hMutex );
	}
private:
	pthread_mutex_t		m_hMutex;
};

class NX_CAutoLock{
public:
	NX_CAutoLock( NX_CMutex &Lock ) :m_pLock(Lock){
		m_pLock.Lock();
	}
    ~NX_CAutoLock() {
        m_pLock.Unlock();
    };
protected:
	NX_CMutex& m_pLock;
private:
	NX_CAutoLock (const NX_CAutoLock &Ref);
	NX_CAutoLock &operator=(NX_CAutoLock &Ref);
};

class NX_CAutoLock2{
public:
	NX_CAutoLock2( NX_CMutex *Lock ){
		m_pLock = Lock;
		m_pLock->Lock();
	}
    ~NX_CAutoLock2() {
        m_pLock->Unlock();
    };
protected:
	NX_CMutex *m_pLock;
private:
	NX_CAutoLock2 (const NX_CAutoLock2 &Ref);
	NX_CAutoLock2 &operator=(NX_CAutoLock2 &Ref);
};

class NX_CEvent{
public:
	NX_CEvent()
	{
		pthread_cond_init( &m_hCond, NULL );
		pthread_mutex_init( &m_hMutex, NULL );
		m_bWaitExit = false;
	}

	~NX_CEvent()
	{
		pthread_cond_destroy( &m_hCond );
		pthread_mutex_destroy( &m_hMutex );
	}

	int32_t SendEvent()
	{
		pthread_mutex_lock( &m_hMutex );
		pthread_cond_signal( &m_hCond );
		pthread_mutex_unlock( &m_hMutex );
		return 0;
	}

	int32_t WaitEvent()
	{
		pthread_mutex_lock( &m_hMutex );
		pthread_cond_wait( &m_hCond, &m_hMutex );
		pthread_mutex_unlock( &m_hMutex );
		return 0;
	}

	void ResetEvent()
	{
		pthread_mutex_lock( &m_hMutex );
		pthread_cond_broadcast( &m_hCond );
		pthread_mutex_unlock( &m_hMutex );
	}

private:
	pthread_cond_t	m_hCond;
	pthread_mutex_t	m_hMutex;
	bool			m_bWaitExit;
};


//////////////////////////////////////////////////////////////////////////////
//
//							Event Management Classes
//

//
//	Event Receiver
//
class NX_CEventReceiver{
public:
	NX_CEventReceiver(){}
	virtual ~NX_CEventReceiver(){}

	virtual void ProcessEvent( uint32_t EventCode, uint32_t Value ) = 0;

private:
	NX_CEventReceiver (const NX_CEventReceiver &Ref);
	NX_CEventReceiver &operator=(const NX_CEventReceiver &Ref);
};

//
//	Event Notifier
//
class NX_CEventNotifier
{
public:
	NX_CEventNotifier();
	virtual ~NX_CEventNotifier();

public:
	void SendEvent( uint32_t EventCode, uint32_t EventValue );
	void SetEventReceiver( NX_CEventReceiver *pReceiver );

private:
	NX_CEventReceiver	*m_pReceiver;
	NX_CMutex			m_EventLock;
private:
	NX_CEventNotifier (const NX_CEventNotifier &Ref);
	NX_CEventNotifier &operator=(const NX_CEventNotifier &Ref);
};
//
//////////////////////////////////////////////////////////////////////////////


//------------------------------------------------------------------------------
//	NX_CBaseFilter
//------------------------------------------------------------------------------

class	NX_CBaseFilter
{
public:
	NX_CBaseFilter():
		m_bLocked	   ( false ),
		m_pEventNotify ( NULL  ),
		m_pOutFilter   ( NULL  ),
		m_bRunning     ( false ),
		m_pRefClock    ( NULL  )
	{
	}
	virtual ~NX_CBaseFilter(){}

	virtual bool Run()	{	return true;	}
	virtual bool Stop()	{	return true;	}
	virtual bool Flush(){	return true;	}
	virtual NX_CBasePin	*GetPin( int Pos ) = 0;

	//	Clock Reference
	void GetClockRef(NX_CClockRef *pClockRef)
	{
		m_pRefClock = pClockRef;
	}
	void SetClockRef(NX_CClockRef *pClockRef)
	{
		m_pRefClock = pClockRef;
	}
	void SetEventNotifier( NX_CEventNotifier *pEventNotifier )
	{
		m_pEventNotify = pEventNotifier;
	}

	virtual int32_t ReleaseSample( NX_CSample *pSample ) = 0;

public:
	NX_CMutex		m_CtrlLock;
	bool			m_bLocked;
	NX_CEventNotifier *m_pEventNotify;

protected:
	NX_CBaseFilter *m_pOutFilter;
	bool			m_bRunning;
	NX_CClockRef	*m_pRefClock;
};



//------------------------------------------------------------------------------
//	NX_CBasePin
//------------------------------------------------------------------------------
typedef enum{
	PIN_DIRECTION_INPUT,
	PIN_DIRECTION_OUTPUT
}NX_PIN_DIRECTION;

class	NX_CBasePin
{
public:
	NX_CBasePin( NX_CBaseFilter *pOwnerFilter )
	{
		m_pOwnerFilter	= pOwnerFilter;
		m_pPartnerPin	= NULL;
		m_bActive		= false;		//	Pin Status : Inactive
	}
	virtual ~NX_CBasePin( void ){};

	bool	IsConnected()
	{
		return (NULL != m_pPartnerPin)?true:false;
	}

	bool	Connect( NX_CBasePin *pPin )
	{
		if(NULL != pPin && NULL == m_pPartnerPin )
		{
			m_pPartnerPin = pPin;
		}
		return true;
	}

	bool	Disconnect( )
	{
		if( m_pPartnerPin )
		{
			m_pPartnerPin = NULL;
		}
		return true;
	}

	virtual bool Active()
	{
		m_Mutex.Lock();
		m_bActive = true;
		m_Mutex.Unlock();
		return true;
	}

	virtual bool Inactive()
	{
		m_Mutex.Lock();
		m_bActive = false;
		m_Mutex.Unlock();
		return true;
	}

	virtual bool IsActive()
	{
		m_Mutex.Lock();
		bool State = m_bActive;
		m_Mutex.Unlock();
		return State;
	}


protected:
	NX_CBaseFilter		*m_pOwnerFilter;
	NX_CBasePin			*m_pPartnerPin;
	bool				m_bActive;
	NX_CMutex			m_Mutex;
};


class	NX_CBaseOutputPin :
		public NX_CBasePin
{
public:
	NX_CBaseOutputPin( NX_CBaseFilter *pOwnerFilter );
	virtual ~NX_CBaseOutputPin( ){};

	virtual	int32_t		Deliver( NX_CSample *pSample );
	virtual	int32_t		ReleaseSample( NX_CSample *pSample )    = 0;	//	Pure Virtual
	virtual int32_t		GetDeliveryBuffer(NX_CSample **pSample) = 0;	//	Pure Virtual

protected:
	NX_PIN_DIRECTION		m_Direction;
};

class	NX_CBaseInputPin :
		public NX_CBasePin
{
public:
	NX_CBaseInputPin( NX_CBaseFilter *pOwnerFilter );
	virtual ~NX_CBaseInputPin( ){};

	virtual	int32_t		Receive( NX_CSample *pSample ) = 0;
protected:
	NX_PIN_DIRECTION		m_Direction;
};



//------------------------------------------------------------------------------
//	NX_CSample
//------------------------------------------------------------------------------
class	NX_CSample
{
public:
	NX_CSample( NX_CBaseOutputPin *pOwner )
	{
		m_pOwner 		= pOwner;
		m_iRefCount		= 0;
		m_TimeStamp		= -1;
		m_pBuf			= NULL;
		m_BufSize		= 0;
		m_ActualBufSize	= 0;
		m_AlignedByte	= 0;
		m_pNext			= NULL;
		m_bDiscontinuity= false;
		m_bSyncPoint	= false;
		m_bValid		= false;
	}

	virtual ~NX_CSample( void ){
		if( m_pBuf )
			delete[] m_pBuf;
	}

	int32_t 	AddRef( void )
	{
		NX_CAutoLock lock( m_Lock );
		m_iRefCount++;
		return m_iRefCount;
	}
	
	int32_t 	Release( void )
	{
		NX_CAutoLock lock( m_Lock );
		if( m_iRefCount > 0 )
		{
			m_iRefCount--;
			if( m_iRefCount == 0 )
			{
				m_pOwner->ReleaseSample( this );
				return m_iRefCount;
			}
		}
		return m_iRefCount;
	}

	void	SetTime( int64_t TimeStamp )
	{
		m_TimeStamp = TimeStamp;
	}
	
	int32_t		GetTime( int64_t *TimeStamp )
	{
		if( m_TimeStamp == -1 )
			return -1;
		*TimeStamp = m_TimeStamp;
		return 0;		//	NO ERROR
	}

	void	SetBuffer( void *pBuf, int32_t Size, int32_t Align )
	{
		m_pBuf = pBuf;
		m_BufSize = Size;
		m_AlignedByte = Align;
	}

	int32_t GetPointer(void** ppBuf)
	{
		if (NULL != m_pBuf)
		{
			*ppBuf = (void*)m_pBuf;
			return 0;
		}
		return -1;
	}

	int32_t GetPointer(void** ppBuf, int32_t* pSize)
	{
		if (NULL != m_pBuf)
		{
			*ppBuf = (void*)m_pBuf;
			*pSize = m_BufSize;
			return 0;
		}
		return -1;
	}

	int32_t SetActualDataLength ( int32_t Size )
	{
		if( m_BufSize >= Size )
		{
			m_ActualBufSize = Size;
			return 0;
		}
		return -1;
	}

	int32_t GetActualDataLength()
	{
		return m_ActualBufSize;
	}

	int32_t GetRefCount()
	{
		return m_iRefCount;
	}

	bool IsSyncPoint()
	{
		return m_bSyncPoint;
	}

	void SetSyncPoint( bool SyncPoint )
	{
		m_bSyncPoint = SyncPoint;
	}

	void Reset()
	{
		m_iRefCount		= 0;
		m_TimeStamp		= -1;
		m_bDiscontinuity= false;
		m_bSyncPoint	= false;
		m_bValid		= false;
		m_bEndofStream	= false;
	}

	void SetDiscontinuity( bool Discontinuity )
	{
		m_bDiscontinuity = Discontinuity;
	}

	bool IsDiscontinuity( )
	{
		return m_bDiscontinuity;
	}

	bool IsValid()
	{
		return m_bValid;
	}

	void SetValid( bool Valid )
	{
		m_bValid = Valid;
	}

	void SetEndOfStream( bool EndOfStream )
	{
		m_bEndofStream = EndOfStream;
	}

	bool IsEndOfStream()
	{
		return m_bEndofStream;
	}

public:
	NX_CSample *m_pNext;

protected:
	NX_CBaseOutputPin	*m_pOwner;
	int32_t m_iRefCount;
	int64_t	m_TimeStamp;
	void *m_pBuf;
	int32_t m_BufSize;
	int32_t m_ActualBufSize;
	int32_t m_AlignedByte;
	bool m_bDiscontinuity;
	bool m_bSyncPoint;
	bool m_bValid;
	bool m_bEndofStream;
	NX_CMutex m_Lock;

private:
	NX_CSample (const NX_CSample &Ref);
	NX_CSample &operator=(const NX_CSample &Ref);
};

class NX_CVideoSample : public NX_CSample
{
public:
	NX_CVideoSample(NX_CBaseOutputPin* pOwner)
		: NX_CSample(pOwner)
		, m_Width(0)
		, m_Height(0)
	{
	}

	virtual ~NX_CVideoSample(void) {
	}

	void SetImageResolution(int32_t w, int32_t h) {
		m_Width = w;
		m_Height = h;
	}

protected:
	int32_t m_Width;
	int32_t m_Height;
};

class NX_CAudioSample : public NX_CSample
{
public:
	NX_CAudioSample(NX_CBaseOutputPin* pOwner)
		: NX_CSample(pOwner)
		, m_SamplePerBits(16)
		, m_SamplingFreq(44100)
	{
	}

	virtual ~NX_CAudioSample(void) {
	}

protected:
	int32_t m_SamplePerBits;
	int32_t m_SamplingFreq;
};



#if 0

//------------------------------------------------------------------------------
//	NX_CSampleQueue
//------------------------------------------------------------------------------
class	NX_CSampleQueue
{
public:
	NX_CSampleQueue( void )
	{
		m_bEnable = true;
		m_iHeadIndex = 0;
		m_iTailIndex = 0;
		m_iSampleCount = 0;
		m_MaxQueueCnt = SAMPLE_QUEUE_COUNT;

		m_bWaitPush = false;
		pthread_cond_init( &m_hCondPush, NULL );
		pthread_mutex_init( &m_hMutexPush, NULL );

		m_bWaitPop = false;
		pthread_cond_init( &m_hCondPop, NULL );
		pthread_mutex_init( &m_hMutexPop, NULL );
	}

	NX_CSampleQueue( int32_t MaxQueuCnt )
	{
		m_iHeadIndex = 0;
		m_iTailIndex = 0;
		m_iSampleCount = 0;
		m_MaxQueueCnt = MaxQueuCnt;
		m_bWaitPush = false;
		m_bWaitPop = false;
		m_bWaitPush = false;
		pthread_cond_init( &m_hCondPush, NULL );
		pthread_mutex_init( &m_hMutexPush, NULL );

		m_bWaitPop = false;
		pthread_cond_init( &m_hCondPop, NULL );
		pthread_mutex_init( &m_hMutexPop, NULL );
	}

	virtual ~NX_CSampleQueue( void )
	{
		pthread_cond_destroy( &m_hCondPush );
		pthread_mutex_destroy( &m_hMutexPush );
		pthread_cond_destroy( &m_hCondPop );
		pthread_mutex_destroy( &m_hMutexPop );
	}
	
public:	
	virtual int32_t	PushSample( NX_CSample *pSample )
	{
		if( true != m_bEnable )
			return -1;
		if( m_iSampleCount >= m_MaxQueueCnt )
		{

			pthread_mutex_lock( &m_hMutexPush );
			m_bWaitPop = true;
			timespec time;
			int err;
			do{
				clock_gettime(CLOCK_REALTIME, &time);
				if( time.tv_nsec > 700000 )
				{
					time.tv_sec += 1;
					time.tv_nsec -= 700000;
				}else{
					time.tv_nsec += 300000;
				}
				err = pthread_cond_timedwait( &m_hCondPush, &m_hMutexPush, &time );
			}while( ETIMEDOUT == err );
			pthread_mutex_unlock( &m_hMutexPush );
			if( true != m_bEnable  )
				return -1;
		}
		m_Mutex.Lock();
		m_pSamplePool[m_iTailIndex] = pSample;
		m_iTailIndex = (m_iTailIndex+1) % m_MaxQueueCnt;
		m_iSampleCount++;
		m_Mutex.Unlock();
		if( true == m_bWaitPop )
		{
			pthread_mutex_lock( &m_hMutexPop );
			m_bWaitPop = false;
			pthread_cond_signal( &m_hCondPop );
			pthread_mutex_unlock( &m_hMutexPop );
		}
		return 0;
	}
	
	virtual int32_t	PopSample( NX_CSample **ppSample )
	{
		if( true != m_bEnable )
			return -1;
		if( 0 >= m_iSampleCount )
		{
			pthread_mutex_lock( &m_hMutexPop );
			m_bWaitPop = true;
			timespec time;
			int err;
			do{
				clock_gettime(CLOCK_REALTIME, &time);
				if( time.tv_nsec > 700000 )
				{
					time.tv_sec += 1;
					time.tv_nsec -= 700000;
				}else{
					time.tv_nsec += 300000;
				}
				err = pthread_cond_timedwait( &m_hCondPop, &m_hMutexPop, &time);
			}while( ETIMEDOUT == err && true==m_bEnable );
			pthread_mutex_unlock( &m_hMutexPop );
			if( true != m_bEnable  )
				return -1;
		}
		m_Mutex.Lock();
		*ppSample = m_pSamplePool[m_iHeadIndex];
		m_iHeadIndex = (m_iHeadIndex+1) % m_MaxQueueCnt;
		m_iSampleCount--;
		m_Mutex.Unlock();
		if( true == m_bWaitPush )
		{
			pthread_mutex_lock( &m_hMutexPush );
			m_bWaitPush = false;
			pthread_cond_signal( &m_hCondPush );
			pthread_mutex_unlock( &m_hMutexPush );
		}
		return 0;
	}

	//	Resetart Queue
	virtual void RestartQueue( void )
	{
		m_iHeadIndex = 0;
		m_iTailIndex = 0;
		m_iSampleCount = 0;
		m_bEnable = true;
	}

	virtual void EndQueue(void)
	{
		m_bEnable = false;
		m_bWaitPush = false;
		m_bWaitPop = false;
		pthread_mutex_lock( &m_hMutexPop );
		pthread_cond_broadcast( &m_hCondPop );
		pthread_mutex_unlock( &m_hMutexPop );

		pthread_mutex_lock( &m_hMutexPush );
		pthread_cond_broadcast( &m_hCondPush );
		pthread_mutex_unlock( &m_hMutexPush );
	}

	virtual	int	GetSampleCount( void )
	{
		return m_iSampleCount;
	}

private:
	enum { SAMPLE_QUEUE_COUNT = 128 };
	NX_CSample *m_pSamplePool[SAMPLE_QUEUE_COUNT];
	int32_t	m_iHeadIndex, m_iTailIndex, m_iSampleCount;
	int32_t m_MaxQueueCnt;
	bool m_bWaitPush;
	pthread_cond_t m_hCondPush;
	pthread_mutex_t m_hMutexPush;
	bool m_bEnable;
	bool m_bWaitPop;
	pthread_cond_t  m_hCondPop;
	pthread_mutex_t m_hMutexPop;
	NX_CMutex m_Mutex;
};

#else

//------------------------------------------------------------------------------
//	NX_CSampleQueue
//------------------------------------------------------------------------------


class	NX_CSampleQueue
{
public:
	NX_CSampleQueue( int32_t MaxQueueCnt ) :
		m_iHeadIndex( 0 ),
		m_iTailIndex( 0 ),
		m_iSampleCount( 0 ),
		m_MaxQueueCnt( MaxQueueCnt ),
		m_SemPush( NULL ),
		m_SemPop( NULL ),
		m_EndQueue(false)
	{
		m_MaxQueueCnt = MaxQueueCnt;
		m_SemPush = new NX_CSemaphore( MaxQueueCnt, MaxQueueCnt );	//	available buffer
		m_SemPop  = new NX_CSemaphore( MaxQueueCnt, 0           );	//	
		pthread_mutex_init( &m_ValueMutex, NULL );
#ifdef __linux__
		assert( m_SemPush != NULL );
		assert( m_SemPop != NULL );
#endif
	}

	virtual ~NX_CSampleQueue( void )
	{
		if( m_SemPush ){
			m_SemPush->ResetSignal();
			delete m_SemPush;
		}
		if( m_SemPop ){
			m_SemPop->ResetSignal();
			delete m_SemPop;
		}
		pthread_mutex_destroy( &m_ValueMutex );
	}
	
public:	
	virtual int32_t	PushSample( NX_CSample *pSample )
	{
		//pthread_mutex_lock(&m_ValueMutex);
		//if( m_EndQueue ){	pthread_mutex_unlock(&m_ValueMutex);	return -1;}
		//pthread_mutex_unlock(&m_ValueMutex);
		if( m_SemPush->Pend( 300000 ) != 0 )
			return -1;

		pthread_mutex_lock( &m_ValueMutex );
		m_pSamplePool[m_iTailIndex] = pSample;
		m_iTailIndex = (m_iTailIndex+1) % m_MaxQueueCnt;
		m_iSampleCount++;
		pthread_mutex_unlock( &m_ValueMutex );

		return m_SemPop->Post();
	}

	virtual int32_t	PopSample( NX_CSample **ppSample )
	{
		//pthread_mutex_lock(&m_ValueMutex);
		//if( m_EndQueue ){	pthread_mutex_unlock(&m_ValueMutex);	return -1;}
		//pthread_mutex_unlock(&m_ValueMutex);
		if( m_SemPop->Pend( 300000 ) != 0 )
			return -1;

		pthread_mutex_lock( &m_ValueMutex );
		*ppSample = m_pSamplePool[m_iHeadIndex];
		m_iHeadIndex = (m_iHeadIndex+1) % m_MaxQueueCnt;
		m_iSampleCount--;
		pthread_mutex_unlock( &m_ValueMutex );

		return m_SemPush->Post();
	}

	int	GetSampleCount( void )
	{
		int Count;
		pthread_mutex_lock( &m_ValueMutex );
		Count = m_iSampleCount;
		pthread_mutex_unlock( &m_ValueMutex );
		return Count;
	}

	void EndQueue()
	{
		pthread_mutex_lock(&m_ValueMutex);
		m_EndQueue = true;
		m_SemPush->ResetSignal();
		m_SemPop->ResetSignal();
		pthread_mutex_unlock(&m_ValueMutex);
	}

	void ResetQueue()
	{
		pthread_mutex_lock(&m_ValueMutex);
		m_EndQueue = false;
		m_SemPush->ResetValue();
		m_SemPop->ResetValue();
		m_iHeadIndex = 0;
		m_iTailIndex = 0;
		m_iSampleCount = 0;
		pthread_mutex_unlock(&m_ValueMutex);
	}

private:
	enum { SAMPLE_QUEUE_COUNT = 128 };
	NX_CSample *m_pSamplePool[SAMPLE_QUEUE_COUNT];
	int32_t	m_iHeadIndex, m_iTailIndex, m_iSampleCount;
	int32_t m_MaxQueueCnt;
	NX_CSemaphore	*m_SemPush;
	NX_CSemaphore	*m_SemPop;
	pthread_mutex_t	m_ValueMutex;
	bool m_EndQueue;
};


#endif


//
//	Time Resolution :  1/1000 seconds
//
class NX_CClockRef
{
public:
	NX_CClockRef()
		: m_bSetRefTime(false)
		, m_StartTime(0)
		, m_MediaDeltaTime(0)
		, m_PauseDeltaTime(0)
		, m_bStartPuaseTime(false)
	{
	}
	virtual ~NX_CClockRef() {}

public:
	void SetReferenceTime( int64_t StartTime )
	{
		NX_CAutoLock lock(m_Mutex);
		m_bSetRefTime  = true;
		m_StartTime    = StartTime;
		m_RefStartTime = NX_GetTickCount();
	}

	int32_t GetCurrTime( int64_t *CurTime )
	{
		NX_CAutoLock lock(m_Mutex);
		int32_t Ret = -1;
		if( false == m_bSetRefTime ){
			*CurTime = -1;
		}else{
			//
			//	Step 1. Find Reference Time : NX_GetTickCount() - m_RefStartTime
			//	Step 2. Adjust Reference Time to Seek Start Time
			//	Step 3. Adjust Reference Time into m_MediaDeltaTime
			//	Step 4. Adjust Reference Time into m_PauseDeltaTime
			//	
			*CurTime = (NX_GetTickCount() - m_RefStartTime) + m_StartTime + m_MediaDeltaTime - m_PauseDeltaTime;
			Ret = 0;
		}
		return Ret;
	}

	int32_t GetMediaTime( int64_t *CurTime )
	{
		NX_CAutoLock lock(m_Mutex);
		int32_t Ret = -1;
		if( false == m_bSetRefTime ){
			*CurTime = -1;
		}else{
			*CurTime = (NX_GetTickCount() - m_RefStartTime) + m_StartTime + m_MediaDeltaTime - m_PauseDeltaTime;
			if( m_bStartPuaseTime ){
				int64_t tmpTime = NX_GetTickCount();
				*CurTime -= (tmpTime-m_PauseTime);
			}
			Ret = 0;
		}
		return Ret;
	}

	void AdjustMediaTime( int64_t Gap ){
		NX_CAutoLock lock(m_Mutex);
		m_MediaDeltaTime += Gap;
	}

	void PauseRefTime( void ){
		NX_CAutoLock lock(m_Mutex);
		if( m_bStartPuaseTime == false )
		{
			m_PauseTime = NX_GetTickCount();
			m_bStartPuaseTime = true;
		}
	}

	void ResumeRefTime( void ){
		NX_CAutoLock lock(m_Mutex);
		if( m_bStartPuaseTime == true )
		{
			m_ResumeTime = NX_GetTickCount();
			m_PauseDeltaTime += (m_ResumeTime-m_PauseTime);
			m_bStartPuaseTime = false;
		}
	}

	void Reset(){
		NX_CAutoLock lock(m_Mutex);
		m_bSetRefTime = false;
		m_StartTime = 0;
		m_MediaDeltaTime = 0;
		m_PauseDeltaTime = 0;
	}

public:
	bool m_bSetRefTime;

	int64_t	m_StartTime;
	int64_t m_RefStartTime;

	int64_t	m_MediaDeltaTime;
	int64_t m_PauseDeltaTime;

	bool m_bStartPuaseTime;
	int64_t m_PauseTime;
	int64_t m_ResumeTime;

	NX_CMutex m_Mutex;
};



#endif //	__cplusplus

#endif // __NX_CBaseFilter_h__
