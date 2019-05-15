#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>
#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <glib/gstdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "fastcommon/common_define.h"
#include "fastcommon/logger.h"
#include "redispool.h"
#include "janus_fdfs_util.h"
#include "codec/mp3_encoder.h"

#if FDFS_DEMO_TEST_DISABLE
#include "config.h"
#else
#define DEF_JANUS_AUDIO_CACHE_DIR "/tmp/janusCache/audio"
#define DEF_JANUS_VIDEO_CACHE_DIR "/tmp/janusCache/video"
#endif
/* 用于测试程序运行时间 */
#include <time.h>

int process_index = FASTDFS_PROCESS_INDEX;
extern janus_fdfs_context * g_fdfs_context_ptr;

#ifdef TIME_CACULATE
extern unsigned long int insert_num;
extern unsigned long int finish_task;
extern unsigned long int old_num;
extern time_t start;
extern time_t doit;
#endif

#ifdef THREAD_TEST
extern janus_fdfs_info pthread_para;
static gpointer janus_fdfs_dispatch_thread_test(gpointer data);
#endif

static gboolean convert_wav_to_mp3(char *filename);

static char *janus_fdfs_extname_get(char *filename, char delim);

static void janus_fdfs_item_free(gpointer data);

static gboolean janus_fdfs_cache_directory_init(void);

static gpointer janus_fdfs_upload_handler(gpointer data);

static void janus_fdfs_upload_request(gpointer data, gpointer userentity);

static gboolean convert_wav_to_mp3(char *filename) {
    char buf[FILE_NAME_SIZE];

    if (NULL == filename)
        return FALSE;

    get_mp3_file_name(filename, buf, FILE_NAME_SIZE);
    int ret = pcm2mp3(filename, buf, NULL);
    if (0 != ret) {
        JN_DBG_LOG("convert pcm(%s) to mp3(%s) fail, code: %d\n", filename, buf, ret);
        return FALSE;
    }
    /* 删除原wav文件 */
    //g_unlink(filename);
    g_strlcpy(filename, buf, FILE_NAME_SIZE);

    return TRUE;
}

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

/* 
 * ATTENTION:如果路径中存在分隔符delim如'.'，那么有可能分割出错误的字符串
 * e.g. 一个无后缀的文件路径为/a/b/c.d/filename最后就会分割成
 * 文件名:/a/b/c,文件类型名:d/filename,显然是错误的,文件名有类型后缀则不会出问题
 * e.g. /a/b/c.d/filename.mp3就会分割成/a/b/c.d/filename和mp3
 * 当然,路径中没有分隔符则不会出现此类问题
 * 使用basename进行解决,如果这么做,请注意filename指向的字符串会被修改
 */
static char *janus_fdfs_extname_get(char *filename, char delim)
{
    char *delim_ptr = NULL;
    char *base_name = NULL;

    if (NULL == filename || '\0' == delim)
        return NULL;

    if (NULL == (base_name = basename(filename)))
    { /* 非法绝对文件路径 */
        return NULL;
    }
    if (NULL == (delim_ptr = strrchr(base_name, delim)))
    { /* 没找到后缀 */
        return NULL;
    }
    else
    {
        ++delim_ptr;
    }

    return delim_ptr;
}

/* FIXME:后续需要优化成janus的配置 */
static gboolean janus_fdfs_cache_directory_init(void)
{
    int saved_errno;

    /* 创建临时音频存储目录 */
    if (-1 == g_mkdir_with_parents(DEF_JANUS_AUDIO_CACHE_DIR, 0755))
    {
        saved_errno = errno;
        JN_DBG_LOG("Error: %s, Failed to create janus audio directory\n",
            g_strerror(saved_errno));
        return FALSE;
    }

    /* 创建临时视频存储目录 */
    if (-1 == g_mkdir_with_parents(DEF_JANUS_VIDEO_CACHE_DIR, 0755))
    {
        saved_errno = errno;
        JN_DBG_LOG("Error: %s, Failed to create janus audio directory\n",
            g_strerror(saved_errno));
        return FALSE;
    }

    return TRUE;
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

    if (FALSE == janus_fdfs_cache_directory_init())
    {
        JN_DBG_LOG("Failed to create cache directory\n");
        return FALSE;
    }

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

    /* 创建fdfs上传请求消息队列 */
    context->janus_fdfs_requests = g_async_queue_new_full(janus_fdfs_item_free);
    dfs_req_queue_destroy = TRUE;
    
    /* 创建fastDFS上传线程 */
    g_snprintf(fdfs_upload_thr_name, FILE_NAME_SIZE, "fdfs_upload_main_thread");
    context->janus_fdfs_upload_thread = \
                g_thread_try_new(fdfs_upload_thr_name, janus_fdfs_upload_handler, NULL, &error);
    if (NULL != error)
    {
        JN_DBG_LOG("fdfs upload thread start failed, error %d (%s)\n",
                    error->code, error->message ? error->message : "??");
        goto janus_fdfs_failure;
    }
    g_clear_error(&error);
    dfs_main_thread_destroy = TRUE;

    /* 创建线程池并设置线程超时时间 */
    error = NULL;
    context->janus_fdfs_tasks = g_thread_pool_new(janus_fdfs_upload_request, NULL, -1, FALSE, &error);
    if (NULL != error)
    {
        JN_DBG_LOG("dispath thread pool start failed\n");
        goto janus_fdfs_failure;
    }
    g_clear_error(&error);
    /* 2min */
    g_thread_pool_set_max_idle_time(120 * 1000);
    dfs_thread_pool_destroy = TRUE;

    return TRUE;

/* 错误清理 */
janus_fdfs_failure:
    g_clear_error(&error);
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

    /* 用于获取文件类型名：*/
    char *ext_name = NULL;
    char file_url_name[FILE_NAME_SIZE];
    gboolean redis_flag = FALSE;
    int result;
#ifdef TIME_CACULATE
    int timeout = 0;
#endif
    GAsyncQueue *fdfs_request_async_queue = NULL;
    json_t *fdfs_json = NULL;

    g_assert(NULL != g_fdfs_context_ptr);
    fdfs_request_async_queue = g_fdfs_context_ptr->janus_fdfs_requests;

    for ( ; ; ) {
        *storage_ip = '\0';

        entity = (janus_fdfs_info *)g_async_queue_timeout_pop(fdfs_request_async_queue, FDFS_QUEUE_EMPTY_TIMEOUT);
        if (NULL != entity) {
            /* 测试用,上传缓冲区文本 */
            //int store_bytes = strlen(entity->file_name);
            //result = upload_file_by_buff(entity->file_name, store_bytes, entity->file_ext_name, entity->fdfs_url, storage_ip);
            
            /* 上传指定文件,且文件需要指定类型扩展名 */
            if (!is_file_exist(entity->file_path))
            {
                //JANUS_LOG(LOG_WARN, "Error: file \"%s\" not exist\n", entity->file_path);
                JN_DBG_LOG("Error: file \"%s\" not exist\n", entity->file_path);
                janus_fdfs_item_free(entity);
                continue;
            }
            /* 将wav文件转换为mp3文件 */
            if (FALSE == convert_wav_to_mp3(entity->file_path))
            {
                JN_DBG_LOG("Error: failed to get mp3 file\n");
                janus_fdfs_item_free(entity);
                continue;
            }
            /* 注意,如果这个函数会修改file_path,那么最好传一份拷贝进去 */
            ext_name = janus_fdfs_extname_get(entity->file_path, '.');
            if (NULL == ext_name)
            {
                //JANUS_LOG(LOG_WARN, "Error: filename \"%s\" invalid\n", entity->file_path);
                JN_DBG_LOG("Error: filename \"%s\" invalid\n", entity->file_path);
                janus_fdfs_item_free(entity);
                continue;
            }
            result = upload_file_by_filename(entity->file_path, ext_name, file_url_name, storage_ip);
            if (result == 0) //success
            {
                /* 是否拼接成完整的url，这里默认没有 */
                JN_DBG_LOG("==========>>>> upload file %s succefully, get filename url: %s:%d/%s\n", entity->file_path, storage_ip, DEFAULT_SERVICE_PORT, file_url_name);
                //JANUS_LOG(LOG_FATAL, "filename url: %s:%d/%s\n", storage_ip, DEFAULT_SERVICE_PORT, file_url_name);
                /* 准备使用redis连接池写入fastDFS的文件存储信息 */
                /* 增加url信息 */
                fdfs_json = json_object();
                if (NULL != fdfs_json)
                {
                    json_object_set_new(fdfs_json, "uid", json_string(entity->uid));
                    json_object_set_new(fdfs_json, "m_type", json_string(entity->msg_type));
                    json_object_set_new(fdfs_json, "md5", json_string(entity->md5));
                    json_object_set_new(fdfs_json, "grp_id", json_string(entity->grp_id));
                    json_object_set_new(fdfs_json, "timestamp", json_string(entity->timestamp));
                    json_object_set_new(fdfs_json, "file_path", json_string(file_url_name));
                    /* redis工作完毕 */
                    char *temp_string = json_dumps(fdfs_json, JSON_PRESERVE_ORDER);
                    if (NULL != temp_string) {
                        //JANUS_LOG(LOG_DBG, "json value: %s\n", temp_string);
                        JN_DBG_LOG("json value: %s\n", temp_string);
                        redis_flag = janus_push_im_fdfs_url(temp_string);
                        json_decref(fdfs_json);
                        g_free(temp_string);
                    }
    				/* 删除原始数据文件 */
                    //g_unlink(entity->file_path);
                }
                janus_fdfs_item_free(entity);
                if (FALSE == redis_flag)
                {
                    //JANUS_LOG(LOG_WARN, "failed to write fdfs info into redis\n");
                    JN_DBG_LOG("failed to write fdfs info %s into redis\n", entity->file_path);
                    continue;
                }


#ifdef TIME_CACULATE
                /* 测试运行时间 */
                ++finish_task;
                if (finish_task == insert_num && old_num != insert_num)
                {
                    old_num = insert_num;
                    doit = time(NULL);
                    g_printf("\n#############################################################\n");
                    g_printf("########## %lu task finished, time cost: %ld seconds ##########\n", finish_task, doit - start);
                    g_printf("#############################################################\n\n");
                }
#endif
            }
            else //fail
            {
                //LOGD("!!!!!!!! thread:%ld: failed to upload_file: %s\n", syscall(__NR_gettid), entity->file_path);
                /* 重新连接fastdfs */
                dfs_destroy();
                if ((result = dfs_init(process_index, g_fdfs_context_ptr->fdfs_client_conf)) != 0)
                {
                    JN_DBG_LOG("!!!! FATAL Error !!!!: Failed to connect to fastDFS\n");
                }
                else
                {
                    result = upload_file_by_filename(entity->file_path, ext_name, file_url_name, storage_ip);
                    if (0 != result)
                    {
                        LOGD("!!!! FATAL !!!! thread:%ld: failed to upload_file again: %s\n", syscall(__NR_gettid), entity->file_path);
                    }
                }
                /* 删除转换的mp3文件 */
                //g_unlink(entity->file_path);
                janus_fdfs_item_free(entity);
#if 0
                /* 失败的上传推回去重做,这样就不能做上面的free操作 */
                g_async_queue_lock(fdfs_request_async_queue);
                JN_DBG_LOG("thread:%ld: failed to upload_file\n", syscall(__NR_gettid));
                JANUS_LOG(LOG_WARN, "thread:%ld: failed to upload_file\n", syscall(__NR_gettid));
                g_async_queue_push_unlocked(fdfs_request_async_queue, entity);
                g_async_queue_unlock(fdfs_request_async_queue);
#endif
                /* for test */
                g_usleep(100000);
            }


#ifdef TIME_CACULATE
            timeout = 0;
#endif
        }
#ifdef TIME_CACULATE
        else
        {
            //JN_DBG_LOG("failed to get entity, wait\n");
            if (0 != insert_num && insert_num % 1000 == 0)
            {
                JN_DBG_LOG("already finish %d tasks\n", insert_num);
            }
            timeout++;

            /* 自测用退出代码 */
            if (WAIT_TIME_OUT == timeout)
            {
                JN_DBG_LOG("no task now, try to exit\n");
                g_thread_unref(g_thread_self());
            }
        }
#endif
    }

    return NULL;
}

static void janus_fdfs_upload_request(gpointer data, gpointer userdata)
{
    janus_fdfs_info *entity = (janus_fdfs_info *)data;
    GAsyncQueue *fdfs_request_async_queue = NULL;

    g_assert(NULL != g_fdfs_context_ptr);
    fdfs_request_async_queue = g_fdfs_context_ptr->janus_fdfs_requests;

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