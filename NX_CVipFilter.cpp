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
#if defined( _WIN32 )
#include <windows.h>
#include <stdio.h>
#include <string.h>
#elif __linux__
#include <sys/time.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "NX_CVipFilter.h"

#define		DTAG	"[VIP_FILTER]"
#include "NX_DebugMsg.h"

//////////////////////////////////////////////////////////////////////////////
//
//							NX_CSourceFilter
//
NX_CVipFilter::NX_CVipFilter()
	: m_DeviceId(6)
	, m_Width(1280)
	, m_Height(720)
{
	//	Create Audio/Video Output
	m_pVideoOut = new NX_CSourceOutputPin(this, OUT_PIN_TYPE_VIDEO);
	m_NumOutputPin	+= 1;

	m_pReleaseQueue = new NX_CSampleQueue(MAX_VIDEO_NUM_BUF);

	for( int32_t i=0 ; i<MAX_CAMERA_BUFFER; i++ )
		m_hCamMem[i] = 0;

	pthread_mutex_init(&m_hMutexThread, NULL);
}

NX_CVipFilter::~NX_CVipFilter()
{
	if( m_pVideoOut )			delete m_pVideoOut;
	if( m_pReleaseQueue )		delete m_pReleaseQueue;
	pthread_mutex_destroy( &m_hMutexThread );
}


bool NX_CVipFilter::Run()
{
	NX_DbgMsg( DBG_TRACE, DTAG"Run() ++\n" );
	if( m_pVideoOut && m_pVideoOut->IsConnected() ){
		m_pVideoOut->Active();
	}

	if( false == m_bRunning )
	{
		m_bExitThread = false;
		if( pthread_create( &m_hThread, NULL, ThreadStub, this ) < 0 )
		{
			return false;
		}
		m_bRunning = true;
	}
	NX_DbgMsg( DBG_TRACE, DTAG"Run() --\n" );
	return true;
}

bool NX_CVipFilter::Stop()
{
	NX_DbgMsg( DBG_TRACE, "Stop() ++\n" );

	ExitThread();

	if( true == m_bRunning )
	{
		NX_DbgMsg( DBG_TRACE, "Video Out InActive\n" );
		if( m_pVideoOut && m_pVideoOut->IsConnected() )
			m_pVideoOut->Inactive();

		if( m_pReleaseQueue )
			m_pReleaseQueue->EndQueue();

		NX_DbgMsg( DBG_TRACE,  "Main Thread Join ++\n" );
		//	Wait Thread Exit
		pthread_join( m_hThread, NULL );
		NX_DbgMsg( DBG_TRACE, "Main Thread Join --\n" );
		m_bRunning = false;
	}
	NX_DbgMsg( DBG_TRACE, "Stop() --\n" );
	return true;
}

int32_t NX_CVipFilter::ReleaseSample( NX_CSample *pSample )
{
	NX_DbgMsg( DBG_TRACE, DTAG"ReleaseSample() %p, %p\n", pSample, m_pReleaseQueue );
	if( m_pReleaseQueue && pSample )
	{
		m_pReleaseQueue->PushSample( pSample );
		//NX_DbgMsg( DBG_TRACE, DTAG"GetSampleCount() %d\n", m_pReleaseQueue->GetSampleCount() );
	}
	return 0;
}

NX_CBasePin	*NX_CVipFilter::GetPin( int Pos )
{
	if( Pos == 0 )
		return m_pVideoOut;
	return NULL;
}


void *NX_CVipFilter::ThreadStub(void *pObj)
{
	((NX_CVipFilter*)pObj)->ThreadProc();
	return (void*)0;
}


bool NX_CVipFilter::StartThreadProc()
{
	for( int32_t i=0 ; i<MAX_CAMERA_BUFFER; i++ )
	{
		m_hCamMem[i] = NX_AllocateVideoMemory( m_Width , m_Height, 3, V4L2_PIX_FMT_YUV420, 4096 );
		if( m_hCamMem[i] == NULL)
		{
			printf("NX_AllocateVideoMemory Error\n");
			return false;
		}
		NX_MapVideoMemory(m_hCamMem[i]);
	}
	return true;
}

void NX_CVipFilter::EndThreadProc()
{
	for( int32_t i=0 ; i<MAX_CAMERA_BUFFER; i++ )
	{
		if( m_hCamMem[i] )
		{
			NX_FreeVideoMemory( m_hCamMem[i] );
			m_hCamMem[i] = NULL;
		}
	}
}

#define CAMERA_DEVICE		"/dev/video6"
void NX_CVipFilter::ThreadProc()
{
	bool bExit = false;
	int		cameraFd, ret;
	int32_t	bufIdx = -1;
	NX_CSample *pSample;
	NX_CSample *pTmpSample;
	int32_t pixelFormat = V4L2_PIX_FMT_YUV420;
	int32_t busFormat = MEDIA_BUS_FMT_YUYV8_2X8;
	int32_t videoDevice = nx_clipper_video;

	if( !StartThreadProc() )
	{
		NX_ErrMsg( ("Fatal Error. Cannot start Thread.\n") );
		goto THREAD_EXIT;
	}

	//	Open Camera
	cameraFd = open(CAMERA_DEVICE, O_RDWR);
	//	Setting Camera Foramt
	nx_v4l2_set_format(cameraFd, videoDevice, m_Width, m_Height, pixelFormat);
	if( 0 != ret )
	{
		NX_ErrMsg("nx_v4l2_set_format failed !!!\n");
		goto THREAD_EXIT;
	}
	
	//	Request buffer and queue input samples
	ret = nx_v4l2_reqbuf(cameraFd, videoDevice, MAX_CAMERA_BUFFER);
	if (ret) {
		NX_ErrMsg("nx_v4l2_reqbuf failed !!\n");
		goto THREAD_EXIT;
	}

	for ( int32_t i=0; i<MAX_CAMERA_BUFFER ; i++ )
	{
		int32_t size = m_hCamMem[i]->size[0] + m_hCamMem[i]->size[1] + m_hCamMem[i]->size[2];
		ret = nx_v4l2_qbuf(cameraFd, videoDevice, 1, i, &m_hCamMem[i]->sharedFd[0], &size);
		if( 0 != ret )
		{
			NX_ErrMsg("nx_v4l2_qbuf failed !!!\n");
			goto THREAD_EXIT;
		}
	}

	ret = nx_v4l2_streamon(cameraFd, videoDevice);
	if( 0 != ret )
	{
		NX_ErrMsg("nx_v4l2_streamon() failed !!!\n");
		goto THREAD_EXIT;
	}

	while (!bExit)
	{
		pthread_mutex_lock(&m_hMutexThread);
		if (m_bExitThread) { pthread_mutex_unlock(&m_hMutexThread);	break; }
		pthread_mutex_unlock(&m_hMutexThread);

		//	Get Delivery Sample
		m_pVideoOut->GetDeliveryBuffer( &pSample );

		//
		//	queue all buffers
		//
		while(m_pReleaseQueue->GetSampleCount() > 0)
		{
			pTmpSample = NULL;
			m_pReleaseQueue->PopSample(&pTmpSample);
			if( pTmpSample )
			{
				NX_VID_MEMORY_HANDLE pMem = NULL;

				pTmpSample->GetPointer((void**)&pMem);
				if( pMem )
				{
					for( int32_t i=0 ; i<MAX_CAMERA_BUFFER ; i++ )
					{
						if( m_hCamMem[i]->sharedFd[0] == pMem->sharedFd[0] )
						{
							int32_t size = m_hCamMem[i]->size[0] + m_hCamMem[i]->size[1] + m_hCamMem[i]->size[2];
							nx_v4l2_qbuf(cameraFd, videoDevice, 1, i, &m_hCamMem[i]->sharedFd[0], &size);
							NX_DbgMsg( DBG_TRACE, DTAG"nx_v4l2_qbuf (buffer index = %d)\n", i);
						}
					}
				}
			}
		}

		//	Dequeue Buffer
		//NX_DbgMsg( DBG_TRACE, DTAG"nx_v4l2_dqbuf ++\n" );
		nx_v4l2_dqbuf(cameraFd, videoDevice, 1, &bufIdx);
		//NX_DbgMsg( DBG_TRACE, DTAG"nx_v4l2_dqbuf (%d) --\n", bufIdx );

		if( pSample )
		{
			void *pBuf = NULL;
			int32_t bufSize = 0;
			pSample->GetPointer( &pBuf, &bufSize );
			memcpy( pBuf, m_hCamMem[bufIdx], sizeof(NX_VID_MEMORY_INFO) );
			pSample->SetActualDataLength( sizeof(NX_VID_MEMORY_INFO) );
			//NX_DbgMsg( DBG_TRACE, DTAG"Deliver Sample = %d\n", pSample->GetActualDataLength() );
			m_pVideoOut->Deliver(pSample);
			pSample->Release();
		}
	}

	nx_v4l2_streamoff( cameraFd, videoDevice );

THREAD_EXIT:
	if( cameraFd )
	{
		close( cameraFd );
	}


	EndThreadProc();
	NX_DbgMsg( DBG_INFO, "Exit main parsing thread(File)!!\n");
}

void NX_CVipFilter::DeliverEndOfStream( void )
{
	NX_CSourceOutputPin *pOutPin = NULL;

	for( int i=0 ; i<2 ; i++ )
	{
		if		( i==0 )	pOutPin = m_pVideoOut;
		else				pOutPin = NULL;
		

		if( pOutPin && pOutPin->IsConnected() && pOutPin->IsActive() )
		{
			NX_CSample *pSample = NULL;
			pOutPin->m_pInQueue->PopSample( &pSample );
			if( pSample )
			{
				//	Deliver End of Stream
				pSample->Release();
				pSample->SetActualDataLength(0);
				pSample->SetEndOfStream( true );
				pOutPin->m_pOutQueue->PushSample( pSample );
			}
		}
	}
}

//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//
//						Parser Output Pin Class
//

NX_CSourceOutputPin::NX_CSourceOutputPin(NX_CBaseFilter *pOwnerFilter, NX_OUT_PIN_TYPE PinType ):
	NX_CBaseOutputPin( pOwnerFilter ),
	m_PinType( PinType ),
	m_pOutQueue( NULL ),
	m_pSampleList( NULL )
{
	m_pOutQueue = new NX_CSampleQueue(NX_CVipFilter::MAX_CAMERA_BUFFER-1);
	pthread_mutex_init(&m_hMutexThread, NULL);

	AllocBuffers();
}

NX_CSourceOutputPin::~NX_CSourceOutputPin()
{
	if( m_pOutQueue )	delete m_pOutQueue;
	DeallocBuffers();
	pthread_mutex_destroy( &m_hMutexThread );
}

bool NX_CSourceOutputPin::Active()
{
	if( false == m_bActive )
	{
		m_bExitThread = false;
		//	Push Allocated Buffers

		m_pOutQueue->ResetQueue();

		if( m_pOutQueue ){
			NX_CSample *pSample = m_pSampleList;
			while(1)
			{
				if( NULL == pSample )
					break;
				pSample->Reset();
				m_pOutQueue->PushSample( pSample );
				pSample = pSample->m_pNext;
			}
		}

		m_bActive = true;
	}
	return true;
}

bool NX_CSourceOutputPin::Inactive()
{
	//	Set Thread End Command
	m_pOutQueue->EndQueue();
	if( true == m_bActive )
	{
		NX_DbgMsg( DBG_TRACE, "NX_CSourceOutputPin Wait join ++\n" );
		//pthread_join( m_hThread, NULL );
		m_bActive = false;
		NX_DbgMsg( DBG_TRACE, "NX_CSourceOutputPin Wait join --\n" );
	}
	return true;
}


int32_t NX_CSourceOutputPin::ReleaseSample( NX_CSample *pSample )
{
	if( m_pOwnerFilter )
	{
		m_pOwnerFilter->ReleaseSample( pSample );
		m_pOutQueue->PushSample( pSample );
	}
	return 0;
}


int32_t NX_CSourceOutputPin::GetDeliveryBuffer( NX_CSample **ppSample )
{
	*ppSample = NULL;

	if( m_pOutQueue->PopSample( ppSample ) < 0 )
		return -1;
	if( (*ppSample) != NULL )
	{
		(*ppSample)->AddRef();
	}
	return 0;
}

void *NX_CSourceOutputPin::ThreadStub( void *pObj )
{
	((NX_CSourceOutputPin*)pObj)->ThreadProc();
	return (void*)0;
}

void NX_CSourceOutputPin::ThreadProc()
{
	NX_CSample *pOutSample = NULL;

	while(1)
	{
		pthread_mutex_lock( &m_hMutexThread );
		if( m_bExitThread ){	pthread_mutex_unlock( &m_hMutexThread );	break; }
		pthread_mutex_unlock( &m_hMutexThread );

		pOutSample = NULL;
		if( GetDeliveryBuffer(&pOutSample) < 0 )
			break;

		//	Fill Buffer
		if( pOutSample )
		{
			if (Deliver(pOutSample) < 0)
			{
				pOutSample->Release();
				break;
			}
			pOutSample->Release();
		}
	}
	NX_DbgMsg( DBG_INFO, "Exit output pin thread!!(%d)\n", m_PinType );
}

int32_t	NX_CSourceOutputPin::AllocBuffers()
{
	uint32_t *pBuf;
	NX_CSample *pNewSample = NULL;
	int32_t SizeBuf = MAX_VIDEO_SIZE_BUF;
	int32_t NumBuf  = NX_CVipFilter::MAX_CAMERA_BUFFER-1;
	NX_CSample *pOldSample = m_pSampleList;

	int k = 0, i = 0;
	pOldSample = new NX_CVideoSample(this);
	pBuf =  new uint32_t[SizeBuf/sizeof(uint32_t)];
	pOldSample->SetBuffer( pBuf, SizeBuf, sizeof(uint32_t) );
	m_pSampleList = pOldSample;
	k ++;
	for( i=1 ; i<NumBuf ; i++ )
	{
		pNewSample = new NX_CVideoSample(this);
		pBuf =  new uint32_t[SizeBuf/sizeof(uint32_t)];
		pNewSample->SetBuffer( pBuf, SizeBuf, sizeof(uint32_t) );
		pOldSample->m_pNext = pNewSample;
		pOldSample = pNewSample;
		k ++;
	}
	NX_DbgMsg( DBG_INFO, "Allocated Buffers = %d,%d\n", m_PinType, k );
	return 0;
}

int32_t	NX_CSourceOutputPin::DeallocBuffers()
{
	int k =0;
	while(1)
	{
		if( NULL == m_pSampleList )
			break;
		NX_CSample *TmpSampleList = m_pSampleList;
		m_pSampleList = m_pSampleList->m_pNext;
		delete TmpSampleList;
		k++;
	}
	NX_DbgMsg(DBG_INFO, "Deallocated Buffers = %d,%d\n", m_PinType, k );
	return 0;
}
//
//////////////////////////////////////////////////////////////////////////////
