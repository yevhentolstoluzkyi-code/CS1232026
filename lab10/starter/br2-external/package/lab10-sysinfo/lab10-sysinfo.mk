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

# TODO 3. Дві команди. Збирання: make у каталозі джерел $(@D)
# крос-компілятором — змінні середовища й інструменти Buildroot
# дають $(TARGET_MAKE_ENV) і $(TARGET_CONFIGURE_OPTS).
# Встановлення: файл lab10-sysinfo у $(TARGET_DIR)/usr/bin з правами 0755.
define LAB10_SYSINFO_BUILD_CMDS
endef

define LAB10_SYSINFO_INSTALL_TARGET_CMDS
endef

$(eval $(generic-package))
