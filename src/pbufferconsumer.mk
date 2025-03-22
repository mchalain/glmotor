lib-$(HAVE_LIBAVCODEC)+=consumer_ffmpeg
consumer_ffmpeg_SOURCES+=pbufferconsumer/consumer_ffmpeg.c
consumer_ffmpeg_LIBRARY+=libavcodec
consumer_ffmpeg_LIBRARY+=libavutil

lib-$(HAVE_LIBJPEG)+=consumer_libjpeg
consumer_libjpeg_SOURCES+=pbufferconsumer/consumer_libjpeg.c
consumer_libjpeg_LIBRARY+=libjpeg
