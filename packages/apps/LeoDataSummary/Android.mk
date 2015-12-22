LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_PACKAGE_NAME := LeoDataSummary
LOCAL_CERTIFICATE := platform


LOCAL_STATIC_JAVA_LIBRARIES := dataSummary



LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_JAVA_LIBRARIES := telephony-common telephony-msim 
LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_PACKAGE)
##################################################


include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := dataSummary:libs/achartengine-1.1.0.jar

include $(BUILD_MULTI_PREBUILT)

# Use the folloing include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))
