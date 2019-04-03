/*! \file    rediscli.cpp
 * \author   Xiej
 * \copyright GNU General Public License v3
 * \brief    Configuration files parsing (headers)
 * \details  Implementation of a parser of INI and libconfig configuration files.
 * 
 * \ingroup core
 * \ref core
 */

#ifndef JANUS_REDIS_UTILS_H
#define JANUS_REDIS_UTILS_H

#include <hiredis/hiredis.h>
#include <stdio.h>

/* Redis methods */
/* connect to redis */
redisContext* janus_redis_connect_config(const char* config);

/* Connect to redis with definite ip and port */
redisContext* janus_redis_connect_ipAndPort(const char* ip, const int port);

/* Connect to redis whith "AUTH"(password) */
redisContext* janus_redis_connect_all(const char* ip, const int port, const char* auth);

/* redis command */
void* janus_redis_command(redisContext* c, const char* command);

#endif 