/*! \file    rediscli.cpp
 * \author   Xiej
 * \copyright GNU General Public License v3
 * \brief    Configuration files parsing (headers)
 * \details  Implementation of a parser of INI and libconfig configuration files.
 * 
 * \ingroup core
 * \ref core
 */

// typedef struct redisReply {
//     int type;                    /* REDIS_REPLY_* */
//     PORT_LONGLONG integer;       /* The integer when type is REDIS_REPLY_INTEGER */
//     int len;                     /* Length of string */
//     char *str;                   /* Used for both REDIS_REPLY_ERROR and REDIS_REPLY_STRING */
//     size_t elements;             /* number of elements, for REDIS_REPLY_ARRAY */
//     struct redisReply **element; /* elements vector for REDIS_REPLY_ARRAY */
// } redisReply;

#include "config.h"
#include "debug.h"
#include "redis_utils.h"

/* Redis methods */
/* connect to redis */
redisContext* janus_redis_connect_config(const char* config) {
    const char* config_path = "/home/xiejiang/janus-server/etc/janus/";//TODO Change later
    char filename[255] = "/home/xiejiang/janus-server/etc/janus/janus.redis.jcfg";
    // char filename[255];
	// g_snprintf(filename, 255, "%s/%s.jcfg", config_path, "janus.redis");
	// JANUS_LOG(LOG_VERB, "Configuration file: %s\n", filename);
	janus_config *redisconfig = janus_config_parse(filename);
	if(redisconfig == NULL) {
		JANUS_LOG(LOG_WARN, "Couldn't find .jcfg configuration file (%s), trying .cfg\n", "janus.redis");
		g_snprintf(filename, 255, "%s/%s.cfg", config_path, "janus.redis");
		JANUS_LOG(LOG_VERB, "Configuration file: %s\n", filename);
		redisconfig = janus_config_parse(filename);
	}
	// config_folder = config_path;
	if(redisconfig != NULL)
		janus_config_print(redisconfig);

	/* Parse configuration to populate the rooms list */

	if(redisconfig != NULL) {
		janus_config_category *config_redis = janus_config_get_create(redisconfig, NULL, janus_config_type_category, "general");

		const char* redis_ip = NULL;
		guint64 redis_port = 0;
		const char* redis_password = NULL;
		janus_config_item *r_ip = janus_config_get(redisconfig, config_redis, janus_config_type_item, "ip");
		if(r_ip != NULL && r_ip->value != NULL)
			redis_ip = g_strdup(r_ip->value);
		janus_config_item *r_port = janus_config_get(redisconfig, config_redis, janus_config_type_item, "port");
		if(r_port != NULL && r_port->value != NULL)
			redis_port = g_ascii_strtoull(r_port->value, NULL, 0);
		janus_config_item *r_password = janus_config_get(redisconfig, config_redis, janus_config_type_item, "password");
		if(r_password != NULL && r_password->value != NULL)
			redis_password = g_strdup(r_password->value);

        return janus_redis_connect_all(redis_ip, redis_port, redis_password);
    }

    return NULL;
}

/* Connect to redis with definite ip and port */
redisContext* janus_redis_connect_ipAndPort(const char* ip, const int port) {
    redisContext* c = redisConnect(ip, port);
    if (c->err || c == NULL) {
        JANUS_LOG(LOG_WARN, "Connect to redis server fail. \n");
        redisFree(c);
        return NULL;
    }else {
        JANUS_LOG(LOG_INFO, "Connect to redis server success. \n");
    }

    return c;
}

/* Connect to redis whith "AUTH"(password) */
redisContext* janus_redis_connect_all(const char* ip, const int port, const char* auth) {
    redisContext* c = janus_redis_connect_ipAndPort(ip, port);
    if (c != NULL) {
        redisReply* reply = (redisReply*)redisCommand(c, "AUTH %s", auth);
        if (reply->type == REDIS_REPLY_ERROR) {
            JANUS_LOG(LOG_WARN, "Redis authenticate fail. \n");
            redisFree(c);
            return NULL;
        }else {
            JANUS_LOG(LOG_INFO, "Redis authenticate success. \n");
            return c;
        }
    }

    return NULL;
}

/* redis command */
void* janus_redis_command(redisContext* c, const char* command) {
    if (c != NULL) {
        void* reply = redisCommand(c, command);
        return reply;
    }

    return NULL;     
}