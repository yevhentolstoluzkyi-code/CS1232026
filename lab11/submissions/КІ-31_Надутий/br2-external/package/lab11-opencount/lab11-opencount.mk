################################################################################
#
# lab11-opencount
#
################################################################################

LAB11_OPENCOUNT_VERSION = 1.0
LAB11_OPENCOUNT_SITE = $(BR2_EXTERNAL_LAB11_PATH)/package/lab11-opencount/src
LAB11_OPENCOUNT_SITE_METHOD = local
LAB11_OPENCOUNT_LICENSE = GPL-2.0
LAB11_OPENCOUNT_LICENSE_FILES = COPYING

$(eval $(kernel-module))
$(eval $(generic-package))
