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
#ifndef __NX_CVideoEncoderFilter_h__
#define __NX_CVideoEncoderFilter_h__

#include "NX_PlayerConfig.h"

#ifdef __cplusplus
#include "NX_CBaseFilter.h"
#include <pthread.h>

//	Config Media Sample Buffer
#define		MAX_NUM_VIDEO_OUT_BUF		(5)
#define		MAX_SIZE_VIDEO_OUT_BUF		(1024*1024)
#define		MAX_BUFFER_COUNT			(10)

class NX_CVideoDecoderInputPin;
class NX_CVideoDecoderOutputPin;


//////////////////////////////////////////////////////////////////////////////
//
//						Nexell MP3 Decoder Filter
class NX_CVideoEncoderFilter : 
	public NX_CBaseFilter
{
public:
	NX_CVideoEncoderFilter( int32_t Width, int32_t Height, int32_t initQP, int32_t *ErrorCode );
	virtual ~NX_CVideoEncoderFilter();

public:	//	for Video Rendering
	pthread_t			m_hThread;
	bool				m_bExitThread;
	static void			*ThreadStub( void *pObj );
	void				ThreadProc();
	void				ExitThread(){m_bExitThread = true;}
	int32_t				ReleaseSample( NX_CSample *pSample ){return 0;}

	int32_t				SetEncoderInfo( int32_t width, int32_t height, int32_t fps, int32_t gop, uint32_t bitrate, int32_t initQP );

public:
	NX_CBasePin		*GetPin( int Pos );

	//	Override
	virtual bool Run();
	virtual bool Stop();

	//	Overlide from NX_CBaseFilter
	NX_CVideoDecoderInputPin	*m_pInputPin;
	NX_CVideoDecoderOutputPin	*m_pOutputPin;

	NX_CSampleQueue				*m_pInQueue;

private:
	int32_t		m_Width;
	int32_t		m_Height;
	int32_t		m_Fps;
	int32_t		m_Gop;
	uint32_t	m_Bitrate;
	int32_t		m_InitQP;
	int32_t		m_FrameQP;

private:
	NX_CVideoEncoderFilter (NX_CVideoEncoderFilter &Ref);
	NX_CVideoEncoderFilter &operator=(NX_CVideoEncoderFilter &Ref);
};
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//
//						Nexell Video Decoder Output Pin
class NX_CVideoDecoderInputPin : 
	public NX_CBaseInputPin
{
public:
	NX_CVideoDecoderInputPin(NX_CVideoEncoderFilter *pOwner);
	virtual ~NX_CVideoDecoderInputPin(){}

	virtual int32_t Receive( NX_CSample *pInSample );
private:
	NX_CVideoDecoderInputPin (NX_CVideoDecoderInputPin &Ref);
	NX_CVideoDecoderInputPin &operator=(NX_CVideoDecoderInputPin &Ref);
};
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//						Nexell Video Decoder Output Pin
class NX_CVideoDecoderOutputPin : 
	public NX_CBaseOutputPin
{
public:
	NX_CVideoDecoderOutputPin(NX_CVideoEncoderFilter *pOwner);
	virtual ~NX_CVideoDecoderOutputPin();

	//	Implementation Pure Virtual Functions
public:
	virtual int32_t		ReleaseSample( NX_CSample *pSample );
	virtual int32_t		GetDeliveryBuffer(NX_CSample **pSample);

	virtual bool Active();
	virtual bool Inactive();

	//	Alloc Buffer
	int32_t	AllocBuffers();
	int32_t	DeallocBuffers();

	//	Media Sample Queue
	NX_CSampleQueue	*m_pSampleQ;
	NX_CSample		*m_pSampleList;
private:
	NX_CVideoDecoderOutputPin (NX_CVideoDecoderOutputPin &Ref);
	NX_CVideoDecoderOutputPin &operator=(NX_CVideoDecoderOutputPin &Ref);
};
//
//////////////////////////////////////////////////////////////////////////////

#endif	//	__cplusplus

#endif	//	__NX_CVideoEncoderFilter_h__
