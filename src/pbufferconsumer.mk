lib-$(HAVE_LIBAVCODEC)+=consumer_ffmpeg
consumer_ffmpeg_SOURCES+=pbufferconsumer/consumer_ffmpeg.c
consumer_ffmpeg_LIBRARY+=libavcodec
consumer_ffmpeg_LIBRARY+=libavutil
