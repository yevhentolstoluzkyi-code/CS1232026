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

# TODO 5b. Модуль збирає система Kbuild ядра цього образу. Підключіть
# інфраструктуру kernel-module і ту, що робить із каталогу пакет
# (порядок має значення — див. розділ «kernel-module» у Buildroot manual).
