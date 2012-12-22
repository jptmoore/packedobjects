AC_INIT(packedobjects, 0.0.1, jmoore@zedstar.org)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/])
AM_CONFIG_HEADER([src/config.h])

AC_DEFINE([PROGNAME],["packedobjects"],["libpackedobjects commandline XML compression tool"])

# Checks for programs.
AC_PROG_CC

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC

# check for libpackedobjects
PKG_CHECK_MODULES(LIBPACKEDOBJECTS, [libpackedobjects])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_FILES([Makefile src/Makefile])

AC_OUTPUT
