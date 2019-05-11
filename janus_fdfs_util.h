//janus_fdfs_util.h

#ifndef _JANUS_FDFS_UTIL_H
#define _JANUS_FDFS_UTIL_H

#include <glib.h>
#include <jansson.h>
#include "storage/fdfs_api.h"

#define FASTDFS_PROCESS_INDEX 0
#define FDFS_BUF_SIZE 100
#define FILE_NAME_SIZE 256
#define THREAD_SUM 20
#define WAIT_TIME_OUT 10
#define DEFAULT_SERVICE_PORT 82
#define FDFS_QUEUE_EMPTY_TIMEOUT 100000    /* 100ms */

#define DEFAULT_FASTDFS_CONF_FILE "/etc/fdfs/client.conf"
#define DEFAULT_TMP_STORAGE_DIR "/tmp/fastDFS.cache"

//#define TIME_CACULATE

/* 编译测试用例时请置0 */
#define FDFS_DEMO_TEST_DISABLE 1
#if FDFS_DEMO_TEST_DISABLE
#include "debug.h"
#define JN_DBG_LOG(fmt, ...) \
        JANUS_LOG(LOG_INFO, "\n#### JANUS FASTDFS: ####"fmt, ##__VA_ARGS__)
#else
#if 1
#define JN_DBG_LOG(fmt, ...) \
        g_printf(fmt, ##__VA_ARGS__)
#else
#define JN_DBG_LOG(fmt, ...)
#endif
#define JANUS_LOG(fmt, ...)
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
    char file_path[FILE_NAME_SIZE];
    char uid[FDFS_BUF_SIZE];
    char msg_type[FDFS_BUF_SIZE];
    char md5[FDFS_BUF_SIZE];
    char grp_id[FDFS_BUF_SIZE];
    char timestamp[FDFS_BUF_SIZE];
}janus_fdfs_info;

gboolean janus_fdfs_service_init(janus_fdfs_context *context);

void janus_fdfs_service_deinit(janus_fdfs_context *context);

#ifdef __cplusplus
}
#endif

#endif