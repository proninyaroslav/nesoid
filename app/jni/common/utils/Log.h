#ifndef MY_LOG_MACRO_H 
#define MY_LOG_MACRO_H 
#include <android/log.h>
#include <stdio.h>
//redefinition des macros 
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, \
"MY_LOG_TAG", __VA_ARGS__) 
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG  , \
"MY_LOG_TAG", __VA_ARGS__) 
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO   , \
"MY_LOG_TAG", __VA_ARGS__) 
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN   , \
"MY_LOG_TAG", __VA_ARGS__) 
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR  , \
"MY_LOG_TAG", __VA_ARGS__) 
#endif  // MY_LOG_MACRO_H

