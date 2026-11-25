################################################################################
#
# lab10-hello
#
################################################################################

LAB10_HELLO_VERSION = 1.0
LAB10_HELLO_SITE = $(BR2_EXTERNAL_LAB10_PATH)/package/lab10-hello/src
LAB10_HELLO_SITE_METHOD = local
LAB10_HELLO_LICENSE = GPL-2.0
LAB10_HELLO_LICENSE_FILES = COPYING

$(eval $(kernel-module))
$(eval $(generic-package))
