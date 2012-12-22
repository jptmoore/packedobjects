#############################################################
#
# packedobjects
#
 #############################################################
PACKEDOBJECTS_VERSION = 0.0.1
PACKEDOBJECTS_SOURCE = packedobjects-$(PACKEDOBJECTS_VERSION).tar.gz
PACKEDOBJECTS_SITE = http://zedstar.org/tarballs/tools
PACKEDOBJECTS_DEPENDENCIES = libpackedobjects host-pkgconf

$(eval $(autotools-package))
