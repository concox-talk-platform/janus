/**
 * Copyright (c) 2019. All rights reserved.
 * Author: tesion
 * Date: April 22th 2019
 */
#ifndef JANUS_USER_COM_H_
#define JANUS_USER_COM_H_
#include <glib.h>
#include <stdbool.h>

typedef struct user_session {
    guint64 session_id;
} user_session;

typedef enum E_SESSION_ERR {
    E_OK,
    E_USER_EXIST,
    E_UNKNOWN
} E_SESSION_ERR;

bool init_user_session();

bool is_user_exist(guint64 uid);

E_SESSION_ERR  add_user_session(guint64 uid, guint64 session);

bool del_user_session(guint64 uid);

bool del_user_by_session(guint64 session_id);

user_session * get_user_session(guint64 uid);

void release_user_session();

#endif  // JANUS_USER_COM_H_