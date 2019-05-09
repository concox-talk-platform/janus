#ifndef _REDISPOOL_H
#define _REDISPOOL_H

#include <hiredis/hiredis.h>

#define MAX_CONNECTION_NUM 10
#define REDIS_CMD_BUFF_SIZE 256
#define KEY_STRING_SIZE 256

/* redis list key name for instant messages url to store */
#define REDIS_LIST_FDFSURL_KEY "janusImUrlList"

/* use this to initialize a redis list entity */
#define REDIS_INIT_MSG "{ \"info\": \"list initialized\" }"

/* redis-cli execute command: RPUSH LIST_KEY JSON_ELEMENT */
#define RCMD_LIST_INIT_FORMAT "RPUSH %s %s"

/* redis-cli execute command: RPUSHX LIST_KEY JSON_ELEMENT */
#define RCMD_LIST_PUSH_FORMAT "RPUSHX %s %s"

/* 编译测试用例时请置0 */
#define REDIS_DEMO_TEST_DISABLE 1
#if REDIS_DEMO_TEST_DISABLE
#define JN_REDIS_LOG(fmt, ...) \
        JANUS_LOG(LOG_FATAL, "\n#### JANUS REDIS: ####"fmt, ##__VA_ARGS__)
#else
#if 1
#define JN_REDIS_LOG(fmt, ...) \
        g_printf(fmt, ##__VA_ARGS__)
#else
#define JN_REDIS_LOG(fmt, ...)
#endif
#define JANUS_LOG(fmt, ...)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if REDIS_DEMO_TEST_DISABLE
gboolean janus_redispool_init(const char * config);
#else
gboolean janus_redispool_init(void);
#endif

int janus_redispool_destroy(void);

redisContext* janus_get_context(void);

void janus_set_context(redisContext* c);

gboolean janus_push_im_fdfs_url(    const char *json_string);

#ifdef __cplusplus
}
#endif

#endif