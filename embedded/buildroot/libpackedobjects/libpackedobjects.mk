#############################################################
#
# libpackedobjects
#
 #############################################################
LIBPACKEDOBJECTS_VERSION = 0.0.3
LIBPACKEDOBJECTS_SOURCE = libpackedobjects-$(LIBPACKEDOBJECTS_VERSION).tar.gz
LIBPACKEDOBJECTS_SITE = http://zedstar.org/tarballs
LIBPACKEDOBJECTS_INSTALL_STAGING = YES
LIBPACKEDOBJECTS_DEPENDENCIES = libxml2 host-pkgconf

$(eval $(autotools-package))
