AM_CPPFLAGS = -Wall $(LIBXML2_CFLAGS)

lib_LTLIBRARIES = libpackedobjects.la

libpackedobjects_la_LIBADD = $(LIBXML2_LIBS)

libpackedobjects_la_SOURCES = packedobjects.c packedobjects_init.c packedobjects_encode.c packedobjects_decode.c canon.c expand.c schema.c encode.c decode.c ier.c \
	packedobjects.h packedobjects_init.h packedobjects_encode.h packedobjects_decode.h canon.h expand.h schema.h encode.h decode.h ier.h \
	$(top_builddir)/pkgconfig/libpackedobjects.pc \
	$(top_builddir)/schema/packedobjectsDataTypes.xsd $(top_builddir)/schema/packedobjectsSchemaTypes.xsd

library_includedir=$(includedir)/packedobjects
library_include_HEADERS = packedobjects.h packedobjects_init.h packedobjects_encode.h packedobjects_decode.h canon.h expand.h schema.h encode.h decode.h ier.h config.h

check_PROGRAMS = packedobjects
packedobjects_SOURCES = main.c
packedobjects_LDADD = $(lib_LTLIBRARIES) $(LIBXML2_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = $(top_builddir)/pkgconfig/libpackedobjects.pc

libpackedobjectsdir = $(datarootdir)/@PACKAGE@
libpackedobjects_DATA = $(top_builddir)/schema/packedobjectsDataTypes.xsd $(top_builddir)/schema/packedobjectsSchemaTypes.xsd
