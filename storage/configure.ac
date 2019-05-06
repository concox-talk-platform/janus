#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.69)
AC_INIT(g_multiThrandAsyncQ, 1.0, xxx@123.com)
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([fdfsclient], [fdfs_client_init])
AC_CHECK_LIB([fastcommon], [log_init])
AC_CHECK_LIB([glib-2.0], [g_printf])
AC_PROG_RANLIB

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h pthread.h glib.h fastdfs/fdfs_global.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_INT64_T

# Checks for library functions.
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([pthread_create])
AC_CHECK_FUNCS([fdfs_client_init])
AC_CHECK_FUNCS([g_printf])

AC_OUTPUT([Makefile])
