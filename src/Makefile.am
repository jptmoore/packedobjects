INCLUDES = -Wall $(LIBXML2_CFLAGS)

lib_LTLIBRARIES = libpackedobjects.la

libpackedobjects_la_LIBADD = $(LIBXML2_LIBS)

libpackedobjects_la_SOURCES = packedobjects.c canon.c expand.c schema.c encode.c decode.c ier.c \
	packedobjects.h canon.h expand.h schema.h encode.h decode.h ier.h \
	$(top_builddir)/pkgconfig/libpackedobjects.pc

library_includedir=$(includedir)/packedobjects
library_include_HEADERS = packedobjects.h canon.h expand.h schema.h encode.h decode.h ier.h config.h

check_PROGRAMS = packedobjects
packedobjects_SOURCES = main.c
packedobjects_LDADD = $(lib_LTLIBRARIES)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(top_builddir)/pkgconfig/libpackedobjects.pc

libpackedobjectsdir = $(datarootdir)/@PACKAGE@
libpackedobjects_DATA = ../schema/packedobjectsDataTypes.xsd ../schema/packedobjectsSchemaTypes.xsd
