#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "fastcommon/common_define.h"
#include "fastcommon/logger.h"
#include "janus_fdfs_util.h"

/* 用于测试程序运行时间 */
#include <time.h>

int process_index = FASTDFS_PROCESS_INDEX;
extern janus_fdfs_context * g_fdfs_context_ptr;

extern janus_fdfs_info pthread_para;

#ifdef TIME_CACULATE
extern int insert_num;
extern int finish_task;
extern time_t start;
extern time_t doit;
#endif

#ifdef THREAD_TEST
static gpointer janus_fdfs_dispatch_thread_test(gpointer data);
#endif

static gpointer janus_fdfs_upload_handler(gpointer data);

static void janus_fdfs_upload_request(gpointer data, gpointer userentity);

static void janus_fdfs_item_free(gpointer data)
{
    if (NULL != data)
    {
        g_free(data);
    }
}

static gboolean is_file_exist(const char *file_path)
{
    //JN_DBG_LOG("absolute file path: %s\n", file_path);
    return 0 == access(file_path, F_OK);
}

gboolean janus_fdfs_service_init(janus_fdfs_context *context)
{
    int result = -1;
    char fdfs_upload_thr_name[FILE_NAME_SIZE];
    GError *error = NULL;
    
    gboolean dfs_conn_destroy = FALSE;
    gboolean dfs_req_queue_destroy = FALSE;
    gboolean dfs_main_thread_destroy = FALSE;
    gboolean dfs_thread_pool_destroy = FALSE;

    g_assert(NULL != context);
    /* log_init用于fastDFS的日志记录，不可缺少，否则会段错误 */
    log_init();
    /* 建立fdfs通信连接 */
    if ((result = dfs_init(process_index, context->fdfs_client_conf)) != 0)
    {
        JN_DBG_LOG("Error: Failed to connect to fastDFS\n");
        return FALSE;
    }
    dfs_conn_destroy = TRUE;

    JN_DBG_LOG("dfds connection init\n");

    /* 创建fdfs上传请求消息队列 */
    context->janus_fdfs_requests = g_async_queue_new_full(janus_fdfs_item_free);
    dfs_req_queue_destroy = TRUE;

    JN_DBG_LOG("requests queue init\n");
    
    /* 创建fastDFS上传线程 */
    g_snprintf(fdfs_upload_thr_name, FILE_NAME_SIZE, "fdfs_upload_main_thread");
    context->janus_fdfs_upload_thread = \
                g_thread_try_new(fdfs_upload_thr_name, janus_fdfs_upload_handler, NULL, &error);
    if (NULL != error)
    {
        JN_DBG_LOG("fdfs upload thread start failed, error %d (%s)\n",
                    error->code, error->message ? error->message : "??");
        goto fdfs_janus_failure;
    }
    dfs_main_thread_destroy = TRUE;

    JN_DBG_LOG("uploader thread init\n");

    /* 创建线程池并设置线程超时时间 */
    error = NULL;
    context->janus_fdfs_tasks = g_thread_pool_new(janus_fdfs_upload_request, NULL, -1, FALSE, &error);
    if (NULL != error)
    {
        JN_DBG_LOG("dispath thread pool start failed\n");
        goto fdfs_janus_failure;
    }
    /* 2min */
    g_thread_pool_set_max_idle_time(120 * 1000);
    dfs_thread_pool_destroy = TRUE;

    JN_DBG_LOG("request handler pool init\n");

    return TRUE;

/* 错误清理 */
fdfs_janus_failure:
    if (dfs_thread_pool_destroy)
        g_thread_pool_free(context->janus_fdfs_tasks, FALSE, FALSE);
    if (dfs_main_thread_destroy)
        g_thread_unref(context->janus_fdfs_upload_thread);
    if (dfs_req_queue_destroy)
        g_async_queue_unref(context->janus_fdfs_requests);
    if (dfs_conn_destroy)
        dfs_destroy();

    return FALSE;
}

void janus_fdfs_service_deinit(janus_fdfs_context *context)
{
    g_assert(NULL != context);
    g_thread_pool_free(context->janus_fdfs_tasks, FALSE, FALSE);
    /* 放弃队列中的剩余任务 */
    g_thread_unref(context->janus_fdfs_upload_thread);
    g_async_queue_unref(context->janus_fdfs_requests);
    dfs_destroy();

    return;
}

static gpointer janus_fdfs_upload_handler(gpointer data)
{
    janus_fdfs_info *entity;
    char storage_ip[IP_ADDRESS_SIZE];
    char file_path[FILE_NAME_SIZE * 2];
    //int store_bytes = 0;
    int result;
    int timeout = 0;
    GAsyncQueue *fdfs_request_async_queue = NULL;

    g_assert(NULL != g_fdfs_context_ptr);
    fdfs_request_async_queue = g_fdfs_context_ptr->janus_fdfs_requests;

    for ( ; ; ) {
        memset(storage_ip, 0, sizeof(storage_ip));
        *storage_ip = '\0';

        entity = (janus_fdfs_info *)g_async_queue_try_pop(fdfs_request_async_queue);
        if (NULL != entity) {
            /* 测试用,上传缓冲区文本 */
            //store_bytes = strlen(entity->file_name);
            //result = upload_file_by_buff(entity->file_name, store_bytes, entity->file_ext_name, entity->fdfs_url, storage_ip);
            
            /* 上传指定文件,且文件需要指定类型扩展名 */
            g_snprintf(file_path, FILE_NAME_SIZE * 2, "%s/%s.%s", DEFAULT_TMP_STORAGE_DIR, entity->file_name, entity->file_ext_name);
            if (!is_file_exist(file_path))
            {
                JN_DBG_LOG("Error: file \"%s\" not exist\n", file_path);
                janus_fdfs_item_free(entity);
                continue;
            }
            result = upload_file_by_filename(file_path, entity->file_ext_name, entity->fdfs_url, storage_ip);
            if (result == 0) //success
            {
                /* 是否拼接成完整的url，这里默认没有 */
                JN_DBG_LOG("filename url: %s:%d/%s\n", storage_ip, DEFAULT_SERVICE_PORT, entity->fdfs_url);
                /* 后续是否使用异步队列递送给redis处理? */
                entity = NULL;
                janus_fdfs_item_free(entity);
                /* 删除原始数据文件 */
                //g_unlink(file_path);
#ifdef TIME_CACULATE
                /* 测试运行时间 */
                ++finish_task;
                if (finish_task == insert_num)
                {
                    doit = time(NULL);
                    g_printf("\n#############################################################\n");
                    g_printf("########## %d task finished, time cost: %ld seconds ##########\n", finish_task, doit - start);
                    g_printf("#############################################################\n\n");
                }
#endif
            }
            else //fail
            {
                g_async_queue_lock(fdfs_request_async_queue);
                JN_DBG_LOG("thread:%ld: failed to upload_file\n", syscall(__NR_gettid));
                g_async_queue_push_unlocked(fdfs_request_async_queue, entity);
                g_async_queue_unlock(fdfs_request_async_queue);
            }
            timeout = 0;
        }
        else
        {
            //JN_DBG_LOG("failed to get entity, wait\n");
            /* 等50ms */
            g_usleep(50000);
#ifdef TIME_CACULATE
            JN_DBG_LOG("already finish %d tasks\n", insert_num);
            timeout++;

            /* 自测用退出代码 */
            if (WAIT_TIME_OUT == timeout)
            {
                JN_DBG_LOG("no task now, try to exit\n");
                g_thread_unref(g_thread_self());
            }
#endif
        }
    }

    return NULL;
}

static void janus_fdfs_upload_request(gpointer data, gpointer userdata)
{
    janus_fdfs_info *entity = (janus_fdfs_info *)data;
    GAsyncQueue *fdfs_request_async_queue = NULL;

    g_assert(NULL != g_fdfs_context_ptr);
    fdfs_request_async_queue = g_fdfs_context_ptr->janus_fdfs_requests;

    //JN_DBG_LOG("get data: %s %s\n", entity->file_name, entity->file_ext_name);
    g_async_queue_lock(fdfs_request_async_queue);
    g_async_queue_push_unlocked(fdfs_request_async_queue, entity);
#ifdef TIME_CACULATE
    ++process_index;
    ++insert_num;
    JN_DBG_LOG("No.%d task push into queue\n", process_index);
#endif
    g_async_queue_unlock(fdfs_request_async_queue);

    return ;
}

#ifdef THREAD_TEST
static gpointer janus_fdfs_dispatch_thread_test(gpointer data)
{
    janus_fdfs_info *pthread_para = (janus_fdfs_info *)data;
    janus_fdfs_info *entity = NULL;
    int i = 0;
    int j = 0;
    GAsyncQueue *fdfs_request_async_queue = NULL;

    g_assert(NULL != g_fdfs_context_ptr);
    fdfs_request_async_queue = g_fdfs_context_ptr->janus_fdfs_requests;

    while (j < 10) {
        for (i = 0; i < 10; ) {
            entity = (janus_fdfs_info *)g_malloc(sizeof(janus_fdfs_info));
            if (NULL == entity)
            {
                JN_DBG_LOG("Error: failed to allocate memory to store file information\n");
                continue;
            }
            memcpy(entity, &pthread_para[i], sizeof(janus_fdfs_info));
            g_async_queue_lock(fdfs_request_async_queue);
            g_async_queue_push_unlocked(fdfs_request_async_queue, entity);
#ifdef TIME_CACULATE
            ++process_index;
            ++insert_num;
#endif
            JN_DBG_LOG("No.%d task push into queue\n", process_index);
            g_async_queue_unlock(fdfs_request_async_queue);
            ++i;
        }
        ++j;
    }

    return NULL;
}
#endif