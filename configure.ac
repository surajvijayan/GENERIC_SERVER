#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([generic_server], [0.0.0.9], [suraj@broadcom.com])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

#disable static libs
LT_INIT([disable-static])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_LIBTOOL
AM_INIT_AUTOMAKE
AC_PROG_CXX
# Checks for libraries.
AC_CHECK_LIB([ssl], [SSL_library_init])
# Checks for header files.
AC_HEADER_RESOLV
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([inet_ntoa localtime_r memset select socket])
AC_CONFIG_FILES([Makefile
         generic_plugin/Makefile
         framework/Makefile
         sample_client/Makefile
         plugins/sample/Makefile
         CERT/Makefile
         conf/generic_server.conf
         conf/Makefile])
AC_OUTPUT
