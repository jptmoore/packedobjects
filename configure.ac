AC_INIT(libpackedobjects, 0.0.1, jmoore@zedstar.org)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/])
AM_CONFIG_HEADER([src/config.h])

AC_DEFINE([PROGNAME],["libpackedobjects"],["an XML data compression tool"])

AC_DEFINE([NSPREFIX],["xs"],["namespace prefix"])

AC_DEFINE([NSURL],["http://zedstar.org/xml/schema/packedobjectsDataTypes.xsd"],["namespace URL"])

AC_DEFINE([MAX_PDU],[1000],["maximum message size"])


# --enable-debug flag
AC_ARG_ENABLE(debug,
    [  --enable-debug     Turn on debugging [[default=no]]],
    enable_debug="$enableval",
    enable_debug=no)

if test x$enable_debug = xyes ; then
    AC_DEFINE([DEBUG_MODE], [], [true])
fi


# Checks for programs.
AC_PROG_CC

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC

# check for libxml2
PKG_CHECK_MODULES(LIBXML2, [libxml-2.0])

# just create shared library
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_FILES([Makefile src/Makefile])

AC_OUTPUT
