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
#include "NX_PlayerConfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/videodev2.h>
#include <linux/media-bus-format.h>

#include <nx_video_alloc.h>
#include <nx_video_api.h>

#include "NX_CVideoEncoderFilter.h"

#define DTAG "[VID_ENCODER] "
#include "NX_DebugMsg.h"

//////////////////////////////////////////////////////////////////////////////
//
//							NX_CVideoEncoderFilter
//
#define MAX_VIDEO_RESOL_WIDTH		1920
#define MAX_VIDEO_RESOL_HEIGHT		1080
NX_CVideoEncoderFilter::NX_CVideoEncoderFilter( int32_t Width, int32_t Height, int32_t initQP, int32_t *ErrorCode ) : 
	m_bExitThread( true ),
	m_pInputPin ( NULL ),
	m_pOutputPin( NULL ),
	m_Width( Width ),
	m_Height( Height ),
	m_Fps( 30 ),
	m_Gop( 30 ),
	m_Bitrate( 1024*4 ),
	m_InitQP( initQP ),
	m_FrameQP( initQP )
{
	*ErrorCode = 0;
	m_pInputPin		= new NX_CVideoDecoderInputPin( this );
	m_pOutputPin	= new NX_CVideoDecoderOutputPin( this );

	m_pInQueue = new NX_CSampleQueue(64);
}

NX_CVideoEncoderFilter::~NX_CVideoEncoderFilter()
{
	if( m_pInputPin )	delete m_pInputPin;
	if( m_pOutputPin )	delete m_pOutputPin;

	if( m_pInQueue )	delete m_pInQueue ;
}

NX_CBasePin *NX_CVideoEncoderFilter::GetPin( int Pos )
{
	if( 0 == Pos )
		return m_pInputPin;
	if( 1 == Pos )
		return m_pOutputPin;
	return NULL;
}


void *NX_CVideoEncoderFilter::ThreadStub( void *pObj )
{
	if( NULL != pObj )
	{
		((NX_CVideoEncoderFilter*)pObj)->ThreadProc();
	}
	return (void*)(0);
}


int32_t	NX_CVideoEncoderFilter::SetEncoderInfo( int32_t width, int32_t height, int32_t fps, int32_t gop, uint32_t bitrate, int32_t initQP )
{
	NX_DbgMsg( DBG_INFO, "width = %d, height = %d, fps = %d, gop = %d, bitrate = %d, initqp = %d\n", width, height, fps, gop, bitrate, initQP);
	m_Width   = width;
	m_Height  = height;
	m_Fps     = fps;
	m_Gop     = gop;
	m_Bitrate = bitrate;
	m_InitQP   = initQP;
	return 0;
}



void NX_CVideoEncoderFilter::ThreadProc()
{
	NX_CSample *pInSample, *pOutSample;
	uint8_t *pInBuf, *pOutBuf;
	int32_t inSize, outSize;
	uint8_t *pSeqData;
	int32_t seqSize;
	int32_t ret;
	int64_t mediaTime;
	int32_t half = 1;

	bool bInit = false;;

	NX_V4L2ENC_HANDLE hEnc;
	NX_V4L2ENC_PARA encPara;
	uint32_t frameCnt = 0;


	while(1)
	{
		if( m_bExitThread ){ break; }

		//	Input Sample
		pInSample = NULL;
		if( m_pInQueue->PopSample( &pInSample ) <  0)		break;

		if( NULL == pInSample )
			break;

		//	Get Input Buffer Information
		pInBuf = NULL;
		pInSample->GetPointer((void**)&pInBuf);
		inSize = pInSample->GetActualDataLength();
		pInSample->GetTime ( &mediaTime );

		if( bInit == false )
		{
			NX_VID_MEMORY_HANDLE hMem = (NX_VID_MEMORY_HANDLE)pInBuf;
			hEnc = NX_V4l2EncOpen(V4L2_PIX_FMT_H264);
			memset(&encPara, 0, sizeof(encPara));
			encPara.width              = m_Width;
			encPara.height             = m_Height;
			encPara.fpsNum             = m_Fps;
			encPara.fpsDen             = 1;
			encPara.keyFrmInterval     = m_Gop;
			encPara.bitrate            = m_Bitrate * 1024;
			encPara.maximumQp          = 0;
			encPara.rcVbvSize          = 0;
			encPara.disableSkip        = 0;
			encPara.RCDelay            = 0;
			encPara.gammaFactor        = 0;
			encPara.initialQp          = m_InitQP;
			encPara.numIntraRefreshMbs = 0;
			encPara.searchRange        = 0;
			encPara.enableAUDelimiter  = 0;
			encPara.imgFormat          = hMem->format;
			encPara.imgBufferNum       = MAX_BUFFER_COUNT;
			encPara.imgPlaneNum        = hMem->planes;
			encPara.pImage             = hMem;


			NX_DbgMsg( DBG_DEBUG, "GOP = %d\n", m_Gop );
			
			ret = NX_V4l2EncInit(hEnc, &encPara);
			if (ret < 0)
			{
				NX_ErrMsg("video encoder initialization is failed!!!\n");
			}

			ret = NX_V4l2EncGetSeqInfo(hEnc, &pSeqData, &seqSize);
			if (ret < 0)
			{
				NX_ErrMsg("Getting Sequence header is failed!!!\n");
			}
			NX_DbgMsg( DBG_TRACE, DTAG"seqSize = %d\n", seqSize );

			bInit = true;
		}

		//	Encoding
		if( (frameCnt & 0x1) )
		{
			NX_V4L2ENC_IN encIn;
			NX_V4L2ENC_OUT encOut;
			NX_VID_MEMORY_INFO memInfo;
			memcpy( &memInfo, pInBuf, sizeof(memInfo) );

			//encIn.pImage = (NX_VID_MEMORY_HANDLE) pInBuf;
			encIn.pImage = &memInfo;
			encIn.imgIndex = 0;
			encIn.forcedIFrame = 1;
			encIn.forcedSkipFrame = 0;
			encIn.quantParam = m_FrameQP;
			ret = NX_V4l2EncEncodeFrame(hEnc, &encIn, &encOut);
			//	NX_DbgMsg( DBG_TRACE, DTAG"encOut.strmSize= %d\n", encOut.strmSize );
			pInSample->Release();

			if( ret == 0 )
			{
				//	Output Sample
				NX_DbgMsg( DBG_TRACE, DTAG"GetDeliveryBuffer ++\n" );
				if( 0 != m_pOutputPin->GetDeliveryBuffer( &pOutSample ) )
				{
					NX_ErrMsg("GetDeliveryBuffer Failed!!!\n");
					break;
				}
				//	Get Output Buffer Information
				pOutBuf = NULL;
				pOutSample->GetPointer((void**)&pOutBuf);
				//hexdump(encOut.strmBuf, (encOut.strmSize>16)?16:encOut.strmSize );
				if( pOutBuf )
				{
					if( encOut.frameType == 0 )	//	Key Frame
					{
						memcpy( pOutBuf,  pSeqData, seqSize );
						memcpy( pOutBuf + seqSize, encOut.strmBuf, encOut.strmSize );
						pOutSample->SetActualDataLength( encOut.strmSize + seqSize );
						pOutSample->SetSyncPoint( true );
					}
					else
					{
						memcpy( pOutBuf , encOut.strmBuf, encOut.strmSize );
						pOutSample->SetActualDataLength( encOut.strmSize );
						pOutSample->SetSyncPoint( false );
					}
				}
				m_pOutputPin->Deliver(pOutSample);
				pOutSample->Release();
				NX_DbgMsg( DBG_TRACE, DTAG"GetDeliveryBuffer --\n" );
			}
		}
		else
		{
			if( pInSample )
				pInSample->Release();
		}
		frameCnt ++;
	}

EXIT_THREAD:
	if( hEnc )
	{
		NX_V4l2EncClose( hEnc );
	}

	NX_DbgMsg( DBG_INFO, DTAG"Exit encoding thread!!\n" );

}


bool NX_CVideoEncoderFilter::Run()
{
	if( m_pInputPin && m_pInputPin->IsConnected() )
	{
		m_pInputPin->Active();
	}
	if( m_pOutputPin && m_pOutputPin->IsConnected() )
	{
		m_pOutputPin->Active();
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
	return true;
}

bool NX_CVideoEncoderFilter::Stop()
{
	NX_DbgMsg( DBG_TRACE, DTAG"Stop() ++\n" );

	if( m_pInputPin && m_pInputPin->IsConnected() )
	{
		m_pInputPin->Inactive();
	}
	if( m_pOutputPin && m_pOutputPin->IsConnected() )
	{
		m_pOutputPin->Inactive();
	}

	NX_DbgMsg( DBG_TRACE, DTAG"Stop() --\n" );
	return NX_CBaseFilter::Stop();
}


//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
//
//							NX_CVideoDecoderInputPin
//
NX_CVideoDecoderInputPin::NX_CVideoDecoderInputPin(NX_CVideoEncoderFilter *pOwner):
	NX_CBaseInputPin( (NX_CBaseFilter*)pOwner )
{
}

int32_t NX_CVideoDecoderInputPin::Receive( NX_CSample *pInSample )
{
	NX_CAutoLock lock(m_pOwnerFilter->m_CtrlLock);
	if( NULL == m_pOwnerFilter || true != IsActive() )
		return -1;

	pInSample->AddRef();
	return ((NX_CVideoEncoderFilter*)m_pOwnerFilter)->m_pInQueue->PushSample(pInSample);
}

//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
//
//							NX_CVideoDecoderOutputPin
//
NX_CVideoDecoderOutputPin::NX_CVideoDecoderOutputPin(NX_CVideoEncoderFilter *pOwner):
	NX_CBaseOutputPin( (NX_CBaseFilter *)pOwner ),
	m_pSampleQ( NULL ),
	m_pSampleList( NULL )
{
	m_pSampleQ = new NX_CSampleQueue(MAX_NUM_VIDEO_OUT_BUF);
	AllocBuffers();
}

NX_CVideoDecoderOutputPin::~NX_CVideoDecoderOutputPin()
{
	if( m_pSampleQ )
	{
		delete m_pSampleQ;
	}
	DeallocBuffers();
}


bool NX_CVideoDecoderOutputPin::Active()
{
	if( m_pSampleQ )
	{
		m_pSampleQ->ResetQueue();
		NX_CSample *pSample = m_pSampleList;

		for( int i=0 ; i<MAX_NUM_VIDEO_OUT_BUF ; i++ )
		{
			if( NULL != pSample )
			{
				pSample->Reset();
				m_pSampleQ->PushSample( pSample );
				pSample = pSample->m_pNext;
			}
		}
	}

	return NX_CBaseOutputPin::Active();
}

bool NX_CVideoDecoderOutputPin::Inactive()
{
	if( m_pSampleQ )
		m_pSampleQ->EndQueue();
	return NX_CBaseOutputPin::Inactive();
}

int32_t	NX_CVideoDecoderOutputPin::ReleaseSample( NX_CSample *pSample )
{
	NX_DbgMsg(DBG_TRACE, DTAG"ReleaseSample \n");
	m_pSampleQ->PushSample( pSample );
	return 0;
}


int32_t NX_CVideoDecoderOutputPin::GetDeliveryBuffer( NX_CSample **ppSample )
{
	*ppSample = NULL;
	if( m_pSampleQ->PopSample( ppSample ) == 0 ){
		(*ppSample)->AddRef();
		return 0;
	}else{
		return -1;
	}
}

int32_t	NX_CVideoDecoderOutputPin::AllocBuffers()
{
	int k = 0, i=0;
	uint32_t *pBuf;
	NX_CSample *pNewSample = NULL;
	NX_CSample *pOldSample = m_pSampleList;

	pOldSample = new NX_CSample(this);
	pBuf = new uint32_t[MAX_SIZE_VIDEO_OUT_BUF/sizeof(uint32_t)];
	pOldSample->SetBuffer( pBuf, MAX_SIZE_VIDEO_OUT_BUF, sizeof(uint32_t) );
	m_pSampleList = pOldSample;
	k ++;

	for( i=1 ; i<MAX_NUM_VIDEO_OUT_BUF ; i++ )
	{
		pNewSample = new NX_CSample(this);
		pBuf = new uint32_t[MAX_SIZE_VIDEO_OUT_BUF/sizeof(uint32_t)];
		pNewSample->SetBuffer( pBuf, MAX_SIZE_VIDEO_OUT_BUF, sizeof(uint32_t) );
		pOldSample->m_pNext = pNewSample;
		pOldSample = pNewSample;
		k ++;
	}
	NX_DbgMsg( DBG_MEMORY, "[VidEncoder] Allocated Buffers = %d\n", k );
	return 0;
}

int32_t	NX_CVideoDecoderOutputPin::DeallocBuffers()
{
	int k = 0;
	while(1)
	{
		if( NULL == m_pSampleList )
		{
			break;
		}
		NX_CSample *TmpSampleList = m_pSampleList;
		m_pSampleList = m_pSampleList->m_pNext;
		delete TmpSampleList;
		k++;
	}
	NX_DbgMsg( DBG_MEMORY, "[VidEncoder] Deallocated Buffers = %d\n", k );
	return 0;
}

//
//////////////////////////////////////////////////////////////////////////////
