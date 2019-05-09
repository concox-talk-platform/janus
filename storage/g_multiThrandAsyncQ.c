#include <glib.h>
#include <glib/gprintf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "fastcommon/common_define.h"
#include "fastcommon/logger.h"
#include "fdfs_api.h"

#include "../janus_fdfs_util.h"
#include "../redispool.h"

#include <sys/time.h>
#include <time.h>

#define FDFS_CACHE_PATH "/tmp/fastDFS.cache"


GAsyncQueue *g_janus_redispool = NULL;
char g_redis_fdfs_key[KEY_STRING_SIZE] = REDIS_LIST_FDFSURL_KEY;

extern int process_index;
janus_fdfs_context g_fdfs_context =
{
    .fdfs_client_conf = DEFAULT_FASTDFS_CONF_FILE,
    .janus_fdfs_upload_thread = NULL,
    .janus_fdfs_requests = NULL,
    .janus_fdfs_tasks = NULL
};
janus_fdfs_context * g_fdfs_context_ptr = &g_fdfs_context;

#ifdef TIME_CACULATE
int insert_num = 0;
int finish_task = 0;
time_t start;
time_t doit;
#endif

janus_fdfs_info pthread_para[] =
{
    { NULL, NULL },
    { NULL, NULL },
    { NULL, NULL },
    { NULL, NULL },
    { NULL, NULL },
    { NULL, NULL },
    { NULL, NULL },
    { NULL, NULL },
    { NULL, NULL },
    { NULL, NULL }
};

#define OBJ_NUM (sizeof(pthread_para)/sizeof(janus_fdfs_info))

static void test_object_create(void);
static void test_object_destroy(void);
static void fdfs_object_dump(janus_fdfs_info *src);
static void test_dump(void);
static janus_fdfs_info *test_janus_fdfs_info_copy(janus_fdfs_info *dst, janus_fdfs_info *src);


static void test_object_create(void)
{
    int i = 0;
    struct timeval tv;
    char filepath[FILE_NAME_SIZE];
    json_t *tmp = NULL;

    for (i = 0; i < OBJ_NUM; i++)
    {
        gettimeofday(&tv, NULL);
        g_snprintf(filepath, sizeof(filepath), "%s/test%d.txt", FDFS_CACHE_PATH, i + 1);
        pthread_para[i].file_path = g_strdup(filepath);
        pthread_para[i].json_object_ptr = json_object();
        json_object_set_new(pthread_para[i].json_object_ptr, "uid", json_integer(i + 1));
        json_object_set_new(pthread_para[i].json_object_ptr, "type", json_string("ptt"));
        json_object_set_new(pthread_para[i].json_object_ptr, "md5", json_string("to do"));
        json_object_set_new(pthread_para[i].json_object_ptr, "grp_id", json_integer(987654321));
        /* 由fdfs线程写入 */
        //json_object_set_new(fdfs_entity->json_object_ptr, "file_path", json_string(filepath));
        json_object_set_new(pthread_para[i].json_object_ptr, "timestamp", json_integer(tv.tv_usec));
    }
}

static void test_object_destroy(void)
{
    int i = 0;

    for (i = 0; i < OBJ_NUM; i++)
    {
        g_free(pthread_para[i].file_path);
        json_decref(pthread_para[i].json_object_ptr);
    }
}

static void fdfs_object_dump(janus_fdfs_info *src)
{
    char *tmp;

    if (NULL == src)
    {
        g_printf("invalid input pointer\n");
    }

    if (src->file_path)
    {
        g_printf("file path: %s\n", src->file_path);
    }
    else
    {
        g_printf("file path NULL\n");
    }
    if (src->json_object_ptr)
    {
        tmp = json_dumps(src->json_object_ptr, 0);
        g_printf("json object: %s\n", tmp);
        g_free(tmp);
    }
    else
    {
        g_printf("json object NULL\n");
    }

    return;
}

static void test_dump(void)
{
    int i = 0;

    for (i = 0; i < OBJ_NUM; i++)
    {
        fdfs_object_dump(&pthread_para[i]);
    }
}


static janus_fdfs_info *test_janus_fdfs_info_copy(janus_fdfs_info *dst, janus_fdfs_info *src)
{
    dst->file_path = g_strdup(src->file_path);
    dst->json_object_ptr = json_deep_copy(src->json_object_ptr);

    if (NULL == dst->file_path || NULL == dst->json_object_ptr)
    {
        fdfs_object_dump(dst);        
        return NULL;
    }
    else
        return dst;
}

int main(int argc, char **argv)
{
    int result = -1;
    int i = 0, j = 0;
    char thr_name[FILE_NAME_SIZE];
    int thread_num = 0;

#ifdef THREAD_TEST
    GThread **gtid = NULL;
    GError *error = NULL;
#endif
    GError *dispatch_err = NULL;
    janus_fdfs_info *entity = NULL;

    if (2 < argc)
    {
        g_printf("too many input parameter\n");
        g_printf("\nUsage: %s [thread_num]\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    else if (2 == argc)
    {
        thread_num = atoi(argv[1]);
    }
    else
    {}

    thread_num = thread_num ? thread_num : THREAD_SUM;
    test_object_create();
    test_dump();

#ifdef THREAD_TEST
    gtid = (GThread **)g_malloc0(thread_num * sizeof(GThread *));
    if (NULL == gtid)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
#endif

    if (FALSE == janus_redispool_init()) {
        JN_DBG_LOG("Error: janus redis connection init failed\n");
        exit(1);
    }
    JN_DBG_LOG("redispool service initial successfully.\n");
    if (FALSE == janus_fdfs_service_init(g_fdfs_context_ptr))
    {
        JN_DBG_LOG("Error: janus fastDFS service init failed\n");
        exit(EXIT_FAILURE);
    }
    JN_DBG_LOG("fdfs service initial successfully.\n");
#ifdef TIME_CACULATE
    start = time(NULL);
#endif

#ifdef THREAD_TEST
    JN_DBG_LOG("try to start %d dispatch thread\n", OBJ_NUM);
    for (i = 0; i < OBJ_NUM; i++)
    {
        snprintf(thr_name, FILE_NAME_SIZE, "thread_%d.%d", j, i);
        gtid[i] = g_thread_try_new(thr_name, janus_fdfs_dispatch_thread_test, pthread_para, &error);
        if (NULL != error)
        {
            JN_DBG_LOG("No.%d thread start failed, error %d (%s)\n", i, error->code, error->message ? error->message : "??");
            goto ERROR;
        }
        JN_DBG_LOG("No.%d dispatch thread setup\n", i);
    }
#endif

    /* 测试线程池 */
    j = 0;
    for (i = 0; i < thread_num; i++)
    {
        if (10 == j)
            j = 0;
        entity = (janus_fdfs_info *)g_malloc(sizeof(janus_fdfs_info));
        if (NULL == entity)
        {
            JN_DBG_LOG("Error: failed to allocate memory to store file information\n");
            continue;
        }
        if (NULL == test_janus_fdfs_info_copy(entity, &pthread_para[j])) {
            JN_DBG_LOG("Error: failed to copy a new object\n");
            continue;
        }
        JN_DBG_LOG("%d threads alive in pool\n", g_thread_pool_get_num_threads(g_fdfs_context_ptr->janus_fdfs_tasks));

        /* 异步追加请求消息 */
        g_thread_pool_push(g_fdfs_context_ptr->janus_fdfs_tasks, entity, &dispatch_err);
        if (NULL != dispatch_err)
        {
            JN_DBG_LOG("failed to push dispatch task in thread pool, error %d (%s)\n", dispatch_err->code, dispatch_err->message ? dispatch_err->message : "??");
            /* 线程启动错误后应该怎么办?先记录日志,后续再处理 */
        }
        else
        {
            j++;
        }
    }
    g_printf("%d threads alive in pool\n", g_thread_pool_get_num_threads(g_fdfs_context_ptr->janus_fdfs_tasks));

#ifdef THREAD_TEST
    //getchar();
    JN_DBG_LOG("thread over\n");
    for (i = 0; i < OBJ_NUM; i++)
    {
        g_thread_join(gtid[i]);
    }
    free(gtid);
#endif
    g_printf("message thread over, wait for insert thread\n");
    g_printf("%d threads alive in pool\n", g_thread_pool_get_num_threads(g_fdfs_context_ptr->janus_fdfs_tasks));
    getchar();
    janus_fdfs_service_deinit(g_fdfs_context_ptr);
    janus_redispool_destroy();
    test_object_destroy();

    return result;

ERROR:
#ifdef THREAD_TEST
    for (j = 0; j < i; j++)
    {
        g_thread_unref(gtid[i]);
    }
    free(gtid);
#endif

    janus_fdfs_service_deinit(g_fdfs_context_ptr);
	janus_redispool_destroy();
    test_object_destroy();

    return result;
}
