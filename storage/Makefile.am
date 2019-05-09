AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include
bin_PROGRAMS = g_multiThrandAsyncQ
g_multiThrandAsyncQ_SOURCES = g_multiThrandAsyncQ.c fdfs_api.c fdfs_api.h \
							  ../janus_fdfs_util.c ../janus_fdfs_util.h \
							  ../redispool.c ../redispool.h
g_multiThrandAsyncQ_LDFLAGS = -g
g_multiThrandAsyncQ_LDADD = -lpthread -lglib-2.0 -lfdfsclient -lfastcommon -ljansson -lhiredis
