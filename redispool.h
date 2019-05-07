#ifndef _REDISPOOL_H
#define _REDISPOOL_H

#include <hiredis/hiredis.h>

#define MAX_CONNECTION_NUM 10

#ifdef __cplusplus
extern "C" {
#endif
int janus_redispool_init(const char * config);

int janus_redispool_destroy(void);

redisContext* janus_get_context(void);

void janus_set_context(redisContext* c);
#ifdef __cplusplus
}
#endif

#endif