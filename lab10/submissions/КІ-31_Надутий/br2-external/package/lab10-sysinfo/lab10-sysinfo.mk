################################################################################
#
# lab10-sysinfo
#
################################################################################

LAB10_SYSINFO_VERSION = 1.0
LAB10_SYSINFO_SITE = $(BR2_EXTERNAL_LAB10_PATH)/package/lab10-sysinfo/src
LAB10_SYSINFO_SITE_METHOD = local
LAB10_SYSINFO_LICENSE = MIT
LAB10_SYSINFO_LICENSE_FILES = LICENSE

define LAB10_SYSINFO_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D)
endef

define LAB10_SYSINFO_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/lab10-sysinfo $(TARGET_DIR)/usr/bin/lab10-sysinfo
endef

$(eval $(generic-package))
