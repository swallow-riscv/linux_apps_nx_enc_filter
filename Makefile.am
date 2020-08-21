AM_CFLAGS =

AM_CPPFLAGS = \
	$(WARN_CFLAGS)	\
	-I.				\
	-I${includedir}

nx_enc_filter_LDADD =	\
	-lpthread

nx_enc_filter_LDADD +=	\
	-lnx_video_api	\
	-lnx_v4l2

nx_enc_filter_SOURCES = \
	main.cpp \
	NX_DebugMsg.cpp \
	NX_SystemCallLinux.cpp \
	NX_CBaseFilter.cpp \
	NX_CVipFilter.cpp \
	NX_CVideoEncoderFilter.cpp \
	NX_CFileWriteFilter.cpp \
	NX_EncoderManager.cpp

bin_PROGRAMS = nx_enc_filter
