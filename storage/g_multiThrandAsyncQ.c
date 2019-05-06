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

#include <time.h>

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
    { "test1", "txt" },
    { "test2", "txt" },
    { "test3", "txt" },
    { "test4", "txt" },
    { "test5", "txt" },
    { "test6", "txt" },
    { "test7", "txt" },
    { "test8", "txt" },
    { "test9", "txt" },
    { "test10", "txt" }
};

#define OBJ_NUM (sizeof(pthread_para)/sizeof(janus_fdfs_info))

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

#ifdef THREAD_TEST
    gtid = (GThread **)g_malloc0(thread_num * sizeof(GThread *));
    if (NULL == gtid)
    {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
#endif

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
        memcpy(entity, &pthread_para[j], sizeof(janus_fdfs_info));
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

    return result;
}
