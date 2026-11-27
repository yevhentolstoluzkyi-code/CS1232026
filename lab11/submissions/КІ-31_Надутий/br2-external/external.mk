# Підключаємо рецепти всіх пакетів цього дерева.
include $(sort $(wildcard $(BR2_EXTERNAL_LAB11_PATH)/package/*/*.mk))
