//------------------------------------------------------------------------------
//
//	Copyright (C) 2019 Nexell Co. All Rights Reserved
//	Nexell Co. Proprietary & Confidential
//
//	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module		: Simple encoder example
//	File		: main.cpp
//	Description	: This program is a simple example program that is encoded 
//				   from camera and saved as a file.
//	Author		: SeongO Park ( ray@nexell.co.kr )
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>		// for clock()

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ctype.h>

#include <linux/videodev2.h>
#include <linux/media-bus-format.h>

#include <nx-v4l2.h>

#include <nx_video_alloc.h>
#include <nx_video_api.h>


static char gOutputPre[1024] = "/home/root/enc_out";
static uint32_t gBitrate = 4*1024;		//	4Mbps
static uint32_t gGop     = 30;			//	Gop 30 fps
static uint32_t gInitQP  = 25;
static uint32_t gNumGop  = 1;

static uint32_t gMode    = 0;
static int32_t  gWidth   = 1280;
static int32_t  gHeight  = 720;
static int32_t  gFps     = 30;

static void help_opt(int32_t argc, char *argv[])
{
	printf(
		"\n Usage : %s -d [target prefix]\n"
		"  Options :\n"
		"    -m [camera mode]   : 0 = 30fps@HD, 1 = 15fps@VGA (def:0)\n"
		"    -d [target prefix] : output target prefix\n"
		"    -g [Gop]           : GoP value (def:30)\n"
		"    -b [Bitrate]       : encoding bitrate in Kbytes (def:4096(4MB))\n"
		"    -n [number of GoP] : number of GoPs for file dividing. (def:1)\n"
		, argv[0] );
}

static void parse_opt( int32_t argc, char *argv[] )
{
	int32_t opt;
	while (-1 != (opt = getopt(argc, argv, "hd:g:b:n:m:")))
	{
		switch (opt)
		{
			case 'h':
				help_opt(argc, argv);
				exit(0);
			case 'd':
				memset( gOutputPre, 0, sizeof(gOutputPre));
				strcpy( gOutputPre, optarg);
				break;
			case 'g':
				gGop = atoi(optarg);
				break;
			case 'b':
				gBitrate = atoi(optarg);
				break;
			case 'n':
				gNumGop = atoi(optarg);
				break;
			case 'm':
				gMode = atoi(optarg);
				if( gMode == 0 )
				{
					gWidth  = 1280;
					gHeight = 720;
					gFps    = 30;
				}
				else if( gMode == 1 )
				{
					gWidth  = 640;
					gHeight = 480;
					gFps    = 15;
				}
				break;
			default: break;
		}
	}
}

#if 0

#define	MAX_BUFFER_COUNT	(4)
#define CAMERA_DEVICE		"/dev/video6"

int main( int argc, char *argv[] )
{
	//	camera informations
	int32_t module = 0;
	int32_t pixelFormat = V4L2_PIX_FMT_YUV420;
	int32_t busFormat = MEDIA_BUS_FMT_YUYV8_2X8;
	int32_t videoDevice = nx_clipper_video;
	
	int		cameraFd;
	int32_t	bufIdx = -1;

	int32_t	size;
	int		ret;

	char	outFileName[1024];
	
	NX_VID_MEMORY_HANDLE hCamMem[MAX_BUFFER_COUNT] = {0, };
	
	parse_opt( argc, argv );

	//	open camera device
	cameraFd = open(CAMERA_DEVICE, O_RDWR);
	
	printf( "cameraFd = %d\n", cameraFd );

	//	open & initialize camera
	nx_v4l2_set_format(cameraFd, videoDevice, gWidth, gHeight, pixelFormat);
	if( 0 != ret )
	{
		printf("nx_v4l2_set_format failed !!!\n");
		exit(-1);
	}
	
	ret = nx_v4l2_reqbuf(cameraFd, videoDevice, MAX_BUFFER_COUNT);
	if (ret) {
		printf("nx_v4l2_reqbuf failed !!\n");
		exit(-1);
	}


	//	allocate & queue buffer
	for ( int i=0; i<MAX_BUFFER_COUNT ; i++ )
	{
		hCamMem[i] = NX_AllocateVideoMemory( gWidth , gHeight, 3, V4L2_PIX_FMT_YUV420, 4096 );
		if( hCamMem[i] == NULL)
		{
			printf("NX_AllocateVideoMemory Error\n");
			exit (-1);
		}
		NX_MapVideoMemory(hCamMem[i]);

		size = hCamMem[i]->size[0] + hCamMem[i]->size[1] + hCamMem[i]->size[2];
		ret = nx_v4l2_qbuf(cameraFd, videoDevice, 1, i, &hCamMem[i]->sharedFd[0], &size);
		if( 0 != ret )
		{
			printf("nx_v4l2_qbuf failed !!!\n");
			exit(-1);
		}
	}


	//	Encoder
	NX_V4L2ENC_HANDLE hEnc;
	NX_V4L2ENC_PARA encPara;
	uint8_t *pSeqData;
	int32_t seqSize;
	uint32_t frmCnt = 0;
	uint32_t fileCnt = 0;
	
	//	FILE
	clock_t startClock, endClock, clockSum;
	FILE *hFile = NULL;

	hEnc = NX_V4l2EncOpen(V4L2_PIX_FMT_H264);
	memset(&encPara, 0, sizeof(encPara));
	encPara.width              = gWidth;
	encPara.height             = gHeight;
	encPara.fpsNum             = gFps;
	encPara.fpsDen             = 1;
	encPara.keyFrmInterval     = gGop;
	encPara.bitrate            = gBitrate * 1024;
	encPara.maximumQp          = 0;
	encPara.rcVbvSize          = 0;
	encPara.disableSkip        = 0;
	encPara.RCDelay            = 0;
	encPara.gammaFactor        = 0;
	encPara.initialQp          = gInitQP;
	encPara.numIntraRefreshMbs = 0;
	encPara.searchRange        = 0;
	encPara.enableAUDelimiter  = 1;
	encPara.imgFormat          = hCamMem[0]->format;
	encPara.imgBufferNum       = MAX_BUFFER_COUNT;
	encPara.imgPlaneNum        = hCamMem[0]->planes;
	encPara.pImage             = hCamMem[0];
	
	ret = NX_V4l2EncInit(hEnc, &encPara);
	if (ret < 0)
	{
		printf("video encoder initialization is failed!!!\n");
		exit(-1);
	}

	ret = NX_V4l2EncGetSeqInfo(hEnc, &pSeqData, &seqSize);
	if (ret < 0)
	{
		printf("Getting Sequence header is failed!!!\n");
		exit(-1);
	}
	printf( "seqSize = %d\n", seqSize );

	nx_v4l2_streamon(cameraFd, videoDevice);

	if( 0 != ret )
	{
		printf("nx_v4l2_streamon failed !!!\n");
		exit(-1);
	}


	while( 1 )
	{
		NX_V4L2ENC_IN encIn;
		NX_V4L2ENC_OUT encOut;

		if( (frmCnt % (gGop*gNumGop) ) == 0 )
		{
			//	close previous file handle if it's exist.
			if( hFile ){
				fclose( hFile );
			}
			
			//	open file handle
			sprintf( outFileName, "%s_%03d.264", gOutputPre, fileCnt++ );
			printf("File Name : %s\n", outFileName);
			hFile = fopen( outFileName, "wb");
			//	write sps & pps
			if( hFile )
			{
				fwrite( pSeqData, 1, seqSize, hFile );
			}
			clockSum = 0;
		}

		nx_v4l2_dqbuf(cameraFd, videoDevice, 1, &bufIdx);

		memset(&encIn, 0, sizeof(NX_V4L2ENC_IN));
		memset(&encOut, 0, sizeof(NX_V4L2ENC_OUT));

		encIn.pImage = hCamMem[bufIdx];
		encIn.imgIndex = bufIdx;
		encIn.forcedIFrame = 0;
		encIn.forcedSkipFrame = 0;
		encIn.quantParam = 25;

		ret = NX_V4l2EncEncodeFrame(hEnc, &encIn, &encOut);
		if (ret < 0)
		{
			printf("[%d] Frame]NX_V4l2EncEncodeFrame() is failed!!\n", frmCnt);
			break;
		}
		//printf("Size = %5d, Frame Type = %d\n", encOut.strmSize, encOut.frameType);
		if( hFile )
		{
			fwrite( encOut.strmBuf, 1, encOut.strmSize, hFile );
			//hexdump(encOut.strmBuf, (encOut.strmSize<16)? encOut.strmSize : 16);
		}
		nx_v4l2_qbuf(cameraFd, videoDevice, 1, bufIdx, &hCamMem[bufIdx]->sharedFd[0], &size);
		frmCnt ++;
	}

	return 0;
}

#else
#define CAMERA_DEVICE		"/dev/video6"

#include "NX_EncoderManager.h"

int main( int argc, char *argv[] )
{
	NX_EncoderMgr *pMgr;
	parse_opt( argc, argv );

	pMgr = new NX_EncoderMgr( gWidth, gHeight, 34 );

	printf( "~~~~~~~~~~~~~~~~~~~ gWidth(%d), gHeight(%d), gFps(%d), gGop(%d), gBitrate(%d), gInitQP(%d)\n", gWidth, gHeight, gFps, gGop, gBitrate, gInitQP );
	pMgr->SetEncoderInfo( gWidth, gHeight, gFps, gGop, gBitrate, gInitQP );
	pMgr->SetCameraInfo( nx_clipper_video, gWidth, gHeight );
	pMgr->SetFileName( gOutputPre );

	pMgr->Run();

	while(1)
	{
		usleep(1000);
	}

	return 0;
}

#endif