lib-$(HAVE_OPENCV4)+=consumer_ocv
consumer_ocv_SOURCES+=pbufferconsumer/consumer_ocv.cpp
consumer_ocv_LIBRARY+=opencv4

lib-$(HAVE_LIBAVCODEC)+=consumer_ffmpeg
consumer_ffmpeg_SOURCES+=pbufferconsumer/consumer_ffmpeg.c
consumer_ffmpeg_LIBRARY+=libavcodec
consumer_ffmpeg_LIBRARY+=libavutil

lib-$(HAVE_LIBJPEG)+=consumer_libjpeg
consumer_libjpeg_SOURCES+=pbufferconsumer/consumer_libjpeg.c
consumer_libjpeg_LIBRARY+=libjpeg

lib-y+=consumer_socket
consumer_socket_SOURCES+=pbufferconsumer/consumer_socket.c
