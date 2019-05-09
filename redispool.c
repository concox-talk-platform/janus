#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n.h>

#include <jansson.h>
#include "redispool.h"

#if REDIS_DEMO_TEST_DISABLE
#include "config.h"
#include "debug.h"
#endif

extern GAsyncQueue *g_janus_redispool;
extern char g_redis_fdfs_key[KEY_STRING_SIZE];

static gboolean janus_redis_im_fdfs_list_init(    void);

#if REDIS_DEMO_TEST_DISABLE
gboolean janus_redispool_init(const char * config) {
    janus_config *redisconfig = janus_config_parse(config);
	if(redisconfig == NULL) {
		JANUS_LOG(LOG_WARN, "Couldn't find .jcfg configuration file (%s), trying .cfg\n", "janus.transport.redis");
        return FALSE;
	}

    char *ip = NULL;
    guint port = 0;
    char *password = NULL;
	if(redisconfig != NULL) {
		janus_config_category *config_redis = janus_config_get_create(redisconfig, NULL, janus_config_type_category, "general");

		janus_config_item *r_ip = janus_config_get(redisconfig, config_redis, janus_config_type_item, "ip");
		if(r_ip != NULL && r_ip->value != NULL)
			ip = g_strdup(r_ip->value);
		janus_config_item *r_port = janus_config_get(redisconfig, config_redis, janus_config_type_item, "port");
		if(r_port != NULL && r_port->value != NULL)
			port = g_ascii_strtoull(r_port->value, NULL, 0);
		janus_config_item *r_password = janus_config_get(redisconfig, config_redis, janus_config_type_item, "password");
		if(r_password != NULL && r_password->value != NULL)
			password = g_strdup(r_password->value);
        JANUS_LOG(LOG_VERB, "redis info ip %s port %d password %s\n", ip, port, password);
    }
    
    /* FIXME: 后续改造成new_full，增加一个销毁函数 */
    g_janus_redispool = g_async_queue_new();
    int idx;
    for (idx = 0; idx < MAX_CONNECTION_NUM; idx++) {
        redisContext* c = redisConnect(ip, port);
        if (password != NULL) {
            redisReply* reply = (redisReply*)redisCommand(c, "AUTH %s", password);
            if (reply->type == REDIS_REPLY_ERROR) {
                JANUS_LOG(LOG_WARN, "Redis authenticate failed.\n");
                redisFree(c);
                return FALSE;
            }
        }
        g_async_queue_push(g_janus_redispool, c);
    }

    return janus_redis_im_fdfs_list_init();
}
#else
gboolean janus_redispool_init(void) {
    char *ip = "127.0.0.1";
    guint port = 6379;
    char *password = NULL;
    
    /* FIXME: 后续改造成new_full，增加一个销毁函数 */
    g_janus_redispool = g_async_queue_new();
    int idx;
    for (idx = 0; idx < MAX_CONNECTION_NUM; idx++) {
        redisContext* c = redisConnect(ip, port);
        if (password != NULL) {
            redisReply* reply = (redisReply*)redisCommand(c, "AUTH %s", password);
            if (reply->type == REDIS_REPLY_ERROR) {
                JANUS_LOG(LOG_WARN, "Redis authenticate failed.\n");
                redisFree(c);
                return FALSE;
            }
        }
        g_async_queue_push(g_janus_redispool, c);
    }

    return janus_redis_im_fdfs_list_init();
}
#endif

int janus_redispool_destroy(void) {
    int idx;
    for (idx = 0; idx < MAX_CONNECTION_NUM; idx++) {
        redisFree(g_async_queue_try_pop(g_janus_redispool));
    }
    g_async_queue_unref(g_janus_redispool);
    return 0;
}

redisContext* janus_get_context(void) {
    if (g_janus_redispool == NULL){
        return NULL;//TODO Can' t init anywhere, only in main loop
    }
    return g_async_queue_try_pop(g_janus_redispool);
}

void janus_set_context(redisContext* c) {
    g_async_queue_push(g_janus_redispool, c);
}

/* 
 * 初始化redis list对象，以便后续使用pushx指令写入消息
 */
static gboolean janus_redis_im_fdfs_list_init(    void) {
    redisReply *reply = NULL;
    gboolean ret = FALSE;
    redisContext* rcontext = janus_get_context();

    if (NULL == rcontext) {
        JANUS_LOG(LOG_FATAL, "Can't get redispool instance.\n");
        return FALSE;
    }

    /* 检查list是否存在 */
    reply = redisCommand(rcontext, "EXISTS %s", g_redis_fdfs_key);
    if (NULL != reply && reply->type == REDIS_REPLY_INTEGER && 1 == reply->integer) {
		JN_REDIS_LOG("fdfs redis list already initialized\n");
        ret = TRUE;
        freeReplyObject(reply);
	} else { /* list不存在 */
        freeReplyObject(reply);
        /* 命令行中的字符串引号不可缺少 */
        json_t *init = json_object();
        json_object_set_new(init, "info", json_string(REDIS_INIT_MSG));
        char *init_str = json_dumps(init, 0);
        if (NULL != init_str) {
            reply = redisCommand(rcontext, RCMD_LIST_INIT_FORMAT, g_redis_fdfs_key, init_str);
            if (NULL == reply || reply->type == REDIS_REPLY_ERROR) {
        		JANUS_LOG(LOG_FATAL, "redis command reply get failed\n");
                ret = FALSE;
        	} else {
                ret = TRUE;
            }
            freeReplyObject(reply);
            g_free(init_str);
            json_decref(init);
        }
    }
    janus_set_context(rcontext);

    return ret;
}


/* 
 * 调用此函数前需要先构建正确地redis json对象
 * 然后转换成普通的字符串，本函数会将字符串写
 * 入redis
 */
gboolean janus_push_im_fdfs_url(    const char *json_string) {
    redisReply *reply = NULL;
    gboolean ret = TRUE;
    redisContext* rcontext = NULL;

    rcontext = janus_get_context();
    if (NULL == rcontext) {
        JN_REDIS_LOG("Can't get redispool instance.\n");
        return FALSE;
    }

    reply = redisCommand(rcontext, RCMD_LIST_PUSH_FORMAT, g_redis_fdfs_key, json_string);
    if (NULL == reply || reply->type == REDIS_REPLY_ERROR) {
		JN_REDIS_LOG("redis command reply get failed\n");
        ret = FALSE;
	}
    freeReplyObject(reply);
    janus_set_context(rcontext);

    return ret;
}
