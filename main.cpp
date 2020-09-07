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

#include "NX_EncoderManager.h"
#include "NX_DebugMsg.h"

static char gOutputPre[1024] = "/home/root/enc_out";

static bool gEnFileWriting = true;

static uint32_t gBitrate = 4*1024;		//	4Mbps
static uint32_t gGop     = 30;			//	Gop 30 fps
static uint32_t gInitQP  = 25;
static uint32_t gNumGop  = 1;
static uint32_t gNumOutFiles = 1;

static uint32_t gMode    = 0;
static int32_t  gWidth   = 1280;
static int32_t  gHeight  = 720;
static int32_t  gFps     = 30;

static int32_t  gTestTime = 0;

static void help_opt(int32_t argc, char *argv[])
{
	printf(
		"\n Usage : %s -d [target prefix]\n"
		"  Options :\n"
		"    -m [camera mode]         : 0 = 30fps@HD, 1 = 15fps@VGA, 2 = 30fps@540 (def:0)\n"
		"                               3 = 30fps@450, 4=30fps@360\n"
		"    -d [target prefix]       : output target prefix\n"
		"    -g [Gop]                 : GoP value (def:30)\n"
		"    -b [Bitrate]             : encoding bitrate in Kbytes (def:4096(4MB))\n"
		"    -n [number of GoP]       : number of GoPs for file dividing. (def:1)\n"
		"    -f [number of out files] : number of out put files. (def:1)\n"
		, argv[0] );
}

static void parse_opt( int32_t argc, char *argv[] )
{
	int32_t opt;
	while (-1 != (opt = getopt(argc, argv, "t:vhd:g:b:n:m:f:q:w:")))
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
			case 'f':
				gNumOutFiles = atoi(optarg);
				break;
			case 'v':
			{
				int32_t level = atoi(optarg);
				NX_ChangeDebugLevel(level);
				break;
			}
			case 'm':
				gMode = atoi(optarg);
				if( gMode == 0 )
				{
					gWidth  = 1280;
					gHeight = 720;
					gFps    = 30;

					gBitrate = 0;	// VBR
					gGop     = 1;
					gInitQP  = 40;
				}
				else if( gMode == 1 )
				{
					gWidth   = 640;
					gHeight  = 480;
					gFps     = 15;
					gBitrate = 1500;
					gGop     = 15;
					gInitQP  = 25;
				}
				else if( gMode == 2 )
				{
					gWidth   = 960;
					gHeight  = 540;
					gFps     = 30;
					gBitrate = 4096;
					gGop     = 30;
					gInitQP  = 30;
				}
				else if( gMode == 3 )
				{
					gWidth   = 800;
					gHeight  = 450;
					gFps     = 30;
					gBitrate = 4096;
					gGop     = 30;
					gInitQP  = 30;
				}
				else if( gMode == 4 )
				{
					gWidth   = 640;
					gHeight  = 360;
					gFps     = 30;
					gBitrate = 4096;
					gGop     = 30;
					gInitQP  = 30;
				}
				break;
			case 't':
				gTestTime = atoi(optarg);
				break;
			case 'w':
				gEnFileWriting = (atoi(optarg)) ? true : false;
				break;
			case 'q':
				gInitQP = atoi(optarg);
				break;
			default: break;
		}
	}
}

int main( int argc, char *argv[] )
{
	NX_EncoderMgr *pMgr;
	parse_opt( argc, argv );

	pMgr = new NX_EncoderMgr( gWidth, gHeight, gInitQP, 34 );

	//	printf( "gWidth(%d), gHeight(%d), gFps(%d), gGop(%d), gBitrate(%d), gInitQP(%d)\n", gWidth, gHeight, gFps, gGop, gBitrate, gInitQP );
	pMgr->SetEncoderInfo( gWidth, gHeight, gFps, gGop, gBitrate, gInitQP );
	pMgr->SetCameraInfo( nx_clipper_video, gWidth, gHeight );
	pMgr->EnableFileWriting( gEnFileWriting );

	printf( "gOutputPre(%s), gNumFiles(%d), gNumOutFiles(%d)\n", gOutputPre, gNumGop, gNumOutFiles );
	pMgr->SetFileName( gOutputPre, gNumGop, gNumOutFiles );

	pMgr->Run();

	if( gTestTime > 0 )
	{
		for( int32_t i=0 ; i< gTestTime ; i++ )
		{
			usleep(1000000);
		}
	}
	else
	{
		while(1)	
		{
			usleep(10000);
		}
	}
	return 0;
}
