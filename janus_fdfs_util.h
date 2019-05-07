//janus_fdfs_util.h

#ifndef _JANUS_FDFS_UTIL_H
#define _JANUS_FDFS_UTIL_H

#include <glib.h>
#include "storage/fdfs_api.h"
#include "debug.h"

#define FASTDFS_PROCESS_INDEX 0
#define FILE_NAME_SIZE 128
#define THREAD_SUM 20
#define WAIT_TIME_OUT 10
#define DEFAULT_SERVICE_PORT 82

#define DEFAULT_FASTDFS_CONF_FILE "/etc/fdfs/client.conf"
#define DEFAULT_TMP_STORAGE_DIR "/tmp/fastDFS.cache"

//#define TIME_CACULATE

/* 如果想使用demo程序,需要将此处的#if 1替换成0 */
#if 1
#define JN_DBG_LOG(fmt, ...) \
        JANUS_LOG(LOG_FATAL, "\n#### Raphael Said: ####"fmt, ##__VA_ARGS__)
#else
#if 1
#define JN_DBG_LOG(fmt, ...) \
        g_printf(fmt, ##__VA_ARGS__)
#endif
#define JN_DBG_LOG(fmt, ...)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct janus_fdfs_context {
    char fdfs_client_conf[FILE_NAME_SIZE];
    GThread *janus_fdfs_upload_thread;
    GAsyncQueue *janus_fdfs_requests;
    GThreadPool *janus_fdfs_tasks;
}janus_fdfs_context;

typedef struct janus_fdfs_info {
    char file_name[FILE_NAME_SIZE];
    char file_ext_name[FILE_NAME_SIZE];
    char fdfs_url[FILE_NAME_SIZE * 2];
}janus_fdfs_info;

gboolean janus_fdfs_service_init(janus_fdfs_context *context);

void janus_fdfs_service_deinit(janus_fdfs_context *context);

#ifdef __cplusplus
}
#endif

#endif