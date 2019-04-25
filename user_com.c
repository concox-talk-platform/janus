/**
 * Copyright (c) 2019. All rights reserved.
 * Author: tesion
 * Date: April 22th 2019
 * Ps:
 *  user_map是否需要加锁???
 */
#include <stdio.h>
#include "user_com.h"

GHashTable * g_user_map = NULL;

bool init_user_session() {
    if (g_user_map) return true;

    g_user_map = g_hash_table_new_full(g_int64_hash, g_int64_equal, (GDestroyNotify)g_free, (GDestroyNotify)g_free);

    return g_user_map ? true : false;
}

bool is_user_exist(guint64 uid) {
    user_session * sess = g_hash_table_lookup(g_user_map, &uid);
    return sess ? true : false;
}

E_SESSION_ERR add_user_session(guint64 uid, guint64 session_id) {
    // if (is_user_exist(uid)) {
    //     return E_USER_EXIST;
    // }

    user_session * sess = get_user_session(uid);
    if (sess) {
        sess->session_id = session_id;
        return E_OK;
    }

    guint64 * puid = g_malloc(sizeof(guint64));
    *puid = uid;
    user_session * psess = g_malloc(sizeof(user_session));
    psess->session_id = session_id;

    g_hash_table_insert(g_user_map, puid, psess);
    return E_OK;
}

bool del_user_session(guint64 uid) {
    return g_hash_table_remove(g_user_map, &uid);
}

bool del_user_by_session(guint64 session_id) {
    GHashTableIter iter;
    gpointer key, value;
    guint64 * pkey = NULL;
    user_session * pvalue = NULL;
    bool  ret = false;
	g_hash_table_iter_init(&iter, g_user_map);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        pvalue = (user_session *)value;
        if (pvalue && pvalue->session_id == session_id) {
            g_hash_table_iter_remove(&iter);
            // pkey = (guint64 *)key;
            ret = true;
            break;
        }
    }

    // return pkey ? del_user_session(*pkey) : false;
    return ret;
}

user_session * get_user_session(guint64 uid) {
    return g_hash_table_lookup(g_user_map, &uid);
}

void release_user_session() {
    if (!g_user_map) return;

    g_hash_table_remove_all(g_user_map);
    g_user_map = NULL;
}

void user_session_info() {
    GHashTableIter iter;
    gpointer key, value;
    guint64 * pkey = NULL;
    user_session * pvalue = NULL;
    g_hash_table_iter_init(&iter, g_user_map);

    printf("\n<-------------Janus_Session_Info----------->\n");
    printf("UID\t\tSESSION\n");
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        pkey = (guint64 *)key;
        pvalue = (user_session *)value;
    
        if (pvalue) {
            printf("%10llu\t%15llu\n", *pkey, pvalue->session_id);
        }
    }
    printf("------------------End---------------\n");
}
