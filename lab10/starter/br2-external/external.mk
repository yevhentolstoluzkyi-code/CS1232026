# Підключаємо рецепти всіх пакетів цього дерева.
include $(sort $(wildcard $(BR2_EXTERNAL_LAB10_PATH)/package/*/*.mk))
