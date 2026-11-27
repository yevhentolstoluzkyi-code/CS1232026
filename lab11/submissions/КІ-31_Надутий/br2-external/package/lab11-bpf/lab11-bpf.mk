################################################################################
#
# lab11-bpf
#
################################################################################

LAB11_BPF_VERSION = 1.0
LAB11_BPF_SITE = $(BR2_EXTERNAL_LAB11_PATH)/package/lab11-bpf/src
LAB11_BPF_SITE_METHOD = local
LAB11_BPF_LICENSE = GPL-2.0
LAB11_BPF_LICENSE_FILES = COPYING
LAB11_BPF_DEPENDENCIES = libbpf

# BPF byte code is built by clang of the build machine: building
# host-clang in Buildroot would take longer than the whole lab.
# libbpf installs fresh UAPI headers into usr/include/bpf when the
# toolchain headers are older than 6.10, so that directory goes first.
LAB11_BPF_CLANG = clang
LAB11_BPF_BPF_CFLAGS = \
	-I$(STAGING_DIR)/usr/include/bpf \
	-I$(STAGING_DIR)/usr/include
LAB11_BPF_LIBBPF_CFLAGS = -I$(STAGING_DIR)/usr/include/bpf

define LAB11_BPF_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) $(TARGET_CONFIGURE_OPTS) -C $(@D) \
		BPF_CLANG="$(LAB11_BPF_CLANG)" \
		BPF_CFLAGS="$(LAB11_BPF_BPF_CFLAGS)" \
		LIBBPF_CFLAGS="$(LAB11_BPF_LIBBPF_CFLAGS)"
endef

define LAB11_BPF_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/opencount $(TARGET_DIR)/usr/bin/opencount
	$(INSTALL) -D -m 0755 $(@D)/openbench $(TARGET_DIR)/usr/bin/openbench
	$(INSTALL) -D -m 0644 $(@D)/opencount.bpf.o \
		$(TARGET_DIR)/usr/lib/lab11/opencount.bpf.o
	$(INSTALL) -d $(TARGET_DIR)/usr/lib/lab11/verifier
	$(INSTALL) -m 0644 $(@D)/verifier/*.bpf.o $(TARGET_DIR)/usr/lib/lab11/verifier/
endef

$(eval $(generic-package))
