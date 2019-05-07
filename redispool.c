#include <glib.h>
#include "config.h"
#include "debug.h"
#include "redispool.h"

extern GAsyncQueue *g_janus_redispool;

int janus_redispool_init(const char * config) {
    janus_config *redisconfig = janus_config_parse(config);
	if(redisconfig == NULL) {
		JANUS_LOG(LOG_WARN, "Couldn't find .jcfg configuration file (%s), trying .cfg\n", "janus.transport.redis");
        return 0;
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
    

    g_janus_redispool = g_async_queue_new();
    int idx;
    for (idx = 0; idx < MAX_CONNECTION_NUM; idx++) {
        redisContext* c = redisConnect(ip, port);

        if (password != NULL) {
            redisReply* reply = (redisReply*)redisCommand(c, "AUTH %s", password);
            if (reply->type == REDIS_REPLY_ERROR) {
                JANUS_LOG(LOG_WARN, "Redis authenticate failed.\n");
                redisFree(c);
                return 0;
            }
        }
        g_async_queue_push(g_janus_redispool, c);
    }
    
    return 1;
}

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
