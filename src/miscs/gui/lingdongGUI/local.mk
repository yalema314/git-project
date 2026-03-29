# 当前文件所在目录
LOCAL_PATH := $(call my-dir)

ifeq ($(APP_PACK_FLAG),1)  # 生成产物包的时候此组件开源输出
TUYA_APP_OPENSOURCE += $(LOCAL_PATH)
endif
#---------------------------------------

# 清除 LOCAL_xxx 变量
include $(CLEAR_VARS)

ifeq ($(CONFIG_TUYA_LINGDONG_GUI),y)

# 当前模块名
LOCAL_MODULE := $(notdir $(LOCAL_PATH))

# 模块对外头文件（只能是目录）
# 加载至CFLAGS中提供给其他组件使用；开发框架发布此组件开源时不再单独打包至include文件夹；
ifneq ($(APP_PACK_FLAG), 1) 
LOCAL_TUYA_SDK_INC := $(LOCAL_PATH)/include
endif

# 模块对外CFLAGS：其他组件编译时可感知到
LOCAL_TUYA_SDK_CFLAGS := -I$(LOCAL_PATH)/include
LOCAL_TUYA_SDK_CFLAGS += $(addprefix -I ,$(shell find $(LOCAL_PATH)/src/gui -type d))
LOCAL_TUYA_SDK_CFLAGS += $(addprefix -I ,$(shell find $(LOCAL_PATH)/src/misc -type d))

# 模块源代码
LOCAL_SRC_FILES := $(shell find $(LOCAL_PATH)/src/gui -name "*.c" -o -name "*.cpp" -o -name "*.cc")
LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH)/src/misc -name "*.c" -o -name "*.cpp" -o -name "*.cc")

# 模块内部CFLAGS：仅供本组件使用
LOCAL_CFLAGS := -I$(LOCAL_PATH)/include
LOCAL_CFLAGS += $(addprefix -I ,$(shell find $(LOCAL_PATH)/src/gui -type d))
LOCAL_CFLAGS += $(addprefix -I ,$(shell find $(LOCAL_PATH)/src/misc -type d))

# 全局变量赋值
TUYA_SDK_INC += $(LOCAL_TUYA_SDK_INC)  # 此行勿修改
TUYA_SDK_CFLAGS += $(LOCAL_TUYA_SDK_CFLAGS)  # 此行勿修改

ifneq ($(APP_PACK_FLAG),1)  # 生成产物包的时候，开源输出不用再生成静态库
# 生成静态库
include $(BUILD_STATIC_LIBRARY)

# 生成动态库
include $(BUILD_SHARED_LIBRARY)
endif

# 导出编译详情
include $(OUT_COMPILE_INFO)

else
$(info ------ exclude LingDong-GUI component------)
endif # CONFIG_TUYA_LINGDONG_GUI
#---------------------------------------
