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
    if (is_user_exist(uid)) {
        return E_USER_EXIST;
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

user_session * get_user_session(guint64 uid) {
    return g_hash_table_lookup(g_user_map, &uid);
}

void release_user_session() {
    if (!g_user_map) return;

    g_hash_table_remove_all(g_user_map);
    g_user_map = NULL;
}